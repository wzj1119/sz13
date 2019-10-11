#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include "libbson-1.0/bson.h"
#include "libmongoc-1.0/mongoc.h"

#define SavePSize               32
#define SaveMSize               32
#define SaveIdSize              16
#define MAX_MSG_LEN             1024 * 1024*10
#define RECVSIZE                1024 * 1024

uint8_t         m_RecvBuffer[MAX_MSG_LEN];
int             m_nRecvedSize = 0;
uint8_t         pName[SavePSize];
uint8_t         vName[SaveMSize];
uint8_t         saveId[SaveIdSize];
void save15sFile(unsigned char *buffer,int len, char* h15sFileName)
{
    FILE *fp=NULL;
    fp = fopen(h15sFileName,"ab+");
    fwrite(buffer,1,len,fp);
    fclose(fp);
}
// read callback
void read_cb(struct bufferevent *bev, void *arg)
{
    int recvlen = 0;
//    struct evbuffer* buf = (struct evbuffer*)arg;

    // read data from cache
    uint8_t cRecvBuff[RECVSIZE];
    memset(cRecvBuff, 0, RECVSIZE);
    while(( recvlen = bufferevent_read(bev, cRecvBuff, RECVSIZE)) > 0)
    {
        // copy data from cRecvBuff to m_RecvBuffer
        printf("recv DataSize:%d\n",recvlen);
        memcpy(m_RecvBuffer+m_nRecvedSize, cRecvBuff, recvlen);
        m_nRecvedSize += recvlen;
    }
    // sticky packet processing
   while(1)
  {
        int len=0;
        for (len = 0; len < m_nRecvedSize-2; ++len)
        {
           if((m_RecvBuffer[len] ==0x40) && (m_RecvBuffer[len + 1] ==0x40))
           {
              break;
           }
        }
        if(len + 2 >= m_nRecvedSize)
        {
              memset(m_RecvBuffer, 0, MAX_MSG_LEN);
              m_nRecvedSize = 0;
        }
        if(len + 25 >= m_nRecvedSize)
        {
            break;
        }

        int IdIndex=len+2;  //文件ID起始字节
        int Id=(m_RecvBuffer[IdIndex]<<24| m_RecvBuffer[IdIndex+1]<<16 |m_RecvBuffer[IdIndex+2]<<8 | m_RecvBuffer[IdIndex+3]);
       // uint8_t btype=m_RecvBuffer[len+6]&0xFF;
        int btype=m_RecvBuffer[len+6];//文件类型
        int datalenIndex = len + 19;   //数据体长度起始字节
        int datalen=(m_RecvBuffer[datalenIndex]<<24 | m_RecvBuffer[datalenIndex+1]<<16 | m_RecvBuffer[datalenIndex+2]<<8 | m_RecvBuffer[datalenIndex+3]);
        int dataIndex=datalenIndex+4;  //数据体内容起始字节
        if(dataIndex+datalen>m_nRecvedSize)
        {
            break;
        }
        if(0==btype)
        {  
            // save15sFile(m_RecvBuffer+dataIndex, datalen, "/home/wzj/worktest/15swork/file/15stest.jpg");
            memset(pName,0,SavePSize);
           // sprintf(pName,"/home/wzj/worktest/15swork/file/%d.jpg",Id);
            sprintf(pName,"/root/15sMedia/file/%d.jpg",Id);
            save15sFile(m_RecvBuffer+dataIndex, datalen, pName);
            memset(saveId,0,SaveIdSize);
            sprintf(saveId,"%d",Id);
            MongoDB(saveId);
        }
        else if(1==btype)
        {

          // save15sFi  le(m_RecvBuffer+dataIndex,datalen,"/home/wzj/worktest/15swork/file/15stest.mp4");
            memset(vName,0,SaveMSize);
           // sprintf(vName,"/home/wzj/worktest/15swork/file/%d.mp4",Id);
            sprintf(vName,"/root/15sMedia/file/%d.mp4",Id);
            save15sFile(m_RecvBuffer+dataIndex,datalen,vName);
            memset(saveId,0,SaveIdSize);
            sprintf(saveId,"%d",Id);
            MongoDB(saveId);
           // char chBUff[256];
           // memset(chBUff,0,256);
            //sprintf(chBUff,"ffmpeg -i /home/wzj/worktest/15swork/file/%d.mp4 -r 10 -b:v 320k -b:a 32k -y /home/wzj/worktest/15swork/compressfileName/%d.mp4",Id,Id);
            //sprintf(chBUff,"ffmpeg -i /root/15sMedia/file/%d.mp4 -r 10 -b:v 320k -b:a 32k -y /root/15sMedia/compressfileName/%d.mp4",Id,Id);
          //sprintf(chBUff,"ffmpeg -i /home/wzj/worktest/15swork/file/*.mp4 -s 1920x1080 -c:v libx265 -c:a aac -b:v 200k -r 25 /home/wzj/worktest/15swork/compressfileName/%d.mp4",Id);
            //system(chBUff);


          //  system("ffmpeg -threads 4 -i /home/wzj/worktest/15swork/file/*.mp4 -s 1920x1080 -c:v libx265 -c:a aac -b:v 200k -r 25 /home/wzj/worktest/15swork/file/out.mp4");
        }
        else if(2==btype)
        {
             //nothing
        }

      //  memset(saveId,0,SaveIdSize);
     //   sprintf(saveId,"%d",Id);
       // sleep(3000);
    //    MongoDB(saveId);
        memcpy(m_RecvBuffer, m_RecvBuffer +dataIndex+ datalen, m_nRecvedSize);
        m_nRecvedSize -=(dataIndex+datalen);
    }
}
void write_cb(struct bufferevent *bev, void *arg)
{
    uint8_t SendBuffer[6]="@@0##";
    bufferevent_write(bev,SendBuffer,strlen(SendBuffer));
    free(SendBuffer);

}
void event_cb(struct bufferevent *bev, short events, void *arg)
{
    if(events & BEV_EVENT_EOF)
    {
        printf("connnection closed!\n");
    }
    else if(events & BEV_EVENT_ERROR)
    {
        printf("some other error\n");
    }
    bufferevent_free(bev);
}
void listen_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr *addr,
               int len, void *ptr)
{
    printf("new connection!\n");
    struct event_base* base = (struct event_base*)ptr;
    struct bufferevent* bev = NULL;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE); //创建一个bufferevent事件
    //struct evbuffer *buf = evbuffer_new();
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);  //设置回调函数
    bufferevent_enable(bev, EV_READ);  //将读事件加入到监听列表
}
int MongoDB(char *arg)
{
    //QCoreApplication a(argc, argv);
    mongoc_client_t *client;
    mongoc_collection_t *mediaDBcollection;
    mongoc_collection_t *tempDBcollection;
    mongoc_cursor_t *cursor;
    bson_error_t error;
    const bson_t *doc;
    bson_t *query;
    bson_t *bson;
    bson_t opts;
    bson_t child;
    char *str;
    char *string;

    mongoc_init(); //初始化libmongoc驱动
    bson_init(&opts);
    //创建连接对象
    client=mongoc_client_new("mongodb://cesiumei:123456@47.103.85.222:27017/?authSource=media_collection");
    //获取指定数据库和集合
    tempDBcollection=mongoc_client_get_collection(client,"media_collection","temp_collection");
    mediaDBcollection=mongoc_client_get_collection(client,"media_collection","media_collection");
    //query在这里添加一个键值对，查询的时候将只查询出匹配上的结果
    query=bson_new();
    BSON_APPEND_UTF8(query,"id_file",arg);
    BSON_APPEND_DOCUMENT_BEGIN(&opts,"projection",&child);
    BSON_APPEND_INT32(&child,"_id",0);
    bson_append_document_end(&opts,&child);
    //cursor=mongoc_collection_find(tempDBcollection,MONGOC_QUERY_NONE,0,0,0,query,NULL,NULL);
    cursor=mongoc_collection_find_with_opts(tempDBcollection,query,&opts,NULL);

    while(mongoc_cursor_next(cursor,&doc))
    {
        str=bson_as_json(doc,NULL);
       // printf("str:%s\n",str);
        const char *json=str;
        bson=bson_new_from_json((const uint8_t *)json,-1,&error);
        if(!bson)
        {
                fprintf(stderr,"%s\n",error.message);
                return EXIT_FAILURE;
        }
        string=bson_as_json(bson,NULL);
        if (!mongoc_collection_insert (mediaDBcollection, MONGOC_INSERT_NONE, bson, NULL, &error))
        {//插入文档
                fprintf (stderr, "%s\n", error.message);
        }
        bson_destroy(bson);
        bson_free(str);
        bson_free(string);
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(tempDBcollection);//释放表对象；
    mongoc_collection_destroy(mediaDBcollection);
    mongoc_client_destroy(client); //释放连接对形象
    mongoc_cleanup(); //释放libmongoc驱动
    return 0;
}

int main(int argc, char *argv[])
{
   struct event_base* base = event_base_new();

   // init server info
   struct sockaddr_in serv;
   memset(&serv, 0, sizeof(serv));
   serv.sin_family = AF_INET;
   serv.sin_port = htons(21040);
   serv.sin_addr.s_addr = htonl(INADDR_ANY);

   struct evconnlistener* listen = NULL;
   listen = evconnlistener_new_bind(base, listen_cb, base,
                                    LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                    -1, (struct sockaddr*)&serv, sizeof(serv));

   printf("server start...\n");
   // loop
   event_base_dispatch(base);
   //release resource
   evconnlistener_free(listen);
   event_base_free(base);

   return 0;
}
