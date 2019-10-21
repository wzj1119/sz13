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
#include "mongodb.h"

#define SavePSize               256
#define MAX_MSG_LEN             1024 * 1024*10
#define SavedataSize            1024 * 1024*5
#define RECVSIZE                1024 * 1024

typedef struct _stCallback{
    uint8_t         cRecvBuff[RECVSIZE]; //循环接收数据
    uint8_t         m_RecvBuffer[MAX_MSG_LEN];//处理黏包
    int             m_nRecvedSize;      //接收到的数据大小   
    int             m_RecvdataSize;    //数据体大小
    uint8_t         Savedata[SavedataSize]; //存数据体
    uint8_t         pName[SavePSize]; //存名称
}stCallback;

void save15sFile(unsigned char *buffer,int len, char* h15sFileName)
{
    FILE *fp=NULL;
    fp = fopen(h15sFileName,"ab+");
    fwrite(buffer,1,len,fp);
    fclose(fp);
}


void read_cb(struct bufferevent *bev, char*arg)
{
    int recvlen = 0;

    stCallback *callback=(struct stCallback*)arg;

    // read data from cache
    memset(callback->cRecvBuff, 0, RECVSIZE);
    recvlen = bufferevent_read(bev, callback->cRecvBuff, RECVSIZE);
    if(recvlen > 0)
    {
        // copy data from cRecvBuff to m_RecvBuffer
        printf("recv DataSize:%d\n",recvlen);
        memcpy(callback->m_RecvBuffer+callback->m_nRecvedSize, callback->cRecvBuff, recvlen);
        callback->m_nRecvedSize += recvlen;
    }

    //processing
   while(1)
  {
       int len=0;
       if(callback->m_nRecvedSize < 25)
       {
           break;
       }

       //
       if(callback->m_RecvBuffer[len]!=0x40 && callback->m_RecvBuffer[len+1]!=0x40)
       {
           printf("error!\n");
           break;
       }

       int IdIndex=len+2;  //文件ID起始字节

       int Id=(callback->m_RecvBuffer[IdIndex]<<24| callback->m_RecvBuffer[IdIndex+1]<<16 |callback->m_RecvBuffer[IdIndex+2]<<8 | callback->m_RecvBuffer[IdIndex+3]);
       //int btype=callback->m_RecvBuffer[len+6];//文件类型
       int eventType=callback->m_RecvBuffer[len+7];//事件类型

       int datalenIndex = len + 19;   //数据体长度起始字节

     //  printf("%X, %X, %X, %X", callback->m_RecvBuffer[datalenIndex], callback->m_RecvBuffer[datalenIndex+1], callback->m_RecvBuffer[datalenIndex+2], callback->m_RecvBuffer[datalenIndex+3]);
       int datalen=(callback->m_RecvBuffer[datalenIndex]<<24 | callback->m_RecvBuffer[datalenIndex+1]<<16 | callback->m_RecvBuffer[datalenIndex+2]<<8 | callback->m_RecvBuffer[datalenIndex+3]);

    //   if(datalen == 0)
    //   {
    //      printf("end of file\n");
    //   }
      int dataIndex=datalenIndex+4;  //数据体内容起始字节
       printf("len:%d\n",datalen);
       if(25+datalen>callback->m_nRecvedSize)
       {
            break;
       }

       if(datalen>0)
       {
           memcpy(callback->Savedata+callback->m_RecvdataSize,callback->m_RecvBuffer+dataIndex,datalen);
           callback->m_RecvdataSize+=datalen;
       }


      if(datalen==0)
      {
           printf("end of file\n");
           if(2==eventType)
           {
                memset(callback->pName,0,SavePSize);
                sprintf(callback->pName,"/home/wzj/worktest/testproject/file/%d.jpg",Id);
                // sprintf(pName,"/root/15sMedia/file/%d.jpg",Id);
                // save15sFile(callback->Savedata, datalen, callback->pName);
                save15sFile(callback->Savedata,callback->m_RecvdataSize,callback->pName);

                memset(callback->pName,0,SavePSize);
                sprintf(callback->pName,"%d",Id);
                MongoDB(callback->pName);
             //  callback->m_RecvdataSize=0;
            }
            if(0==eventType || 1==eventType)
            {

                memset(callback->pName,0,SavePSize);
                sprintf(callback->pName,"/home/wzj/worktest/testproject/file/%d.mp4",Id);
             // sprintf(vName,"/root/15sMedia/file/%d.mp4",Id);
             // save15sFile(callback->Savedata,datalen,callback->pName);
                save15sFile(callback->Savedata,callback->m_RecvdataSize,callback->pName);

                memset(callback->pName,0,SavePSize);
                sprintf(callback->pName,"%d",Id);
                MongoDB(callback->pName);

                memset(callback->pName,0,SavePSize);
                sprintf(callback->pName,"ffmpeg -i /home/wzj/worktest/testproject/file/%d.mp4 -r 10 -b:v 320k -b:a 32k -y /home/wzj/worktest/testproject/compressfileName/%d.mp4",Id,Id);
                // sprintf(callback->pName,"ffmpeg -i /root/15sMedia/file/%d.mp4 -r 10 -b:v 320k -b:a 32k -y /root/15sMedia/compressfileName/%d.mp4",Id,Id);
                system(callback->pName);
             // callback->m_RecvdataSize=0;
            }

              //   memset(callback->Savedata,0,callback->m_RecvdataSize);
             callback->m_RecvdataSize=0;

      }

      callback->m_nRecvedSize -=(25+datalen);
      memcpy(callback->m_RecvBuffer, callback->m_RecvBuffer+25 + datalen, callback->m_nRecvedSize);


    }
}

void write_cb(struct bufferevent *bev, char*arg)
{
    char*SendBuffer=arg;
    SendBuffer[6]="@@0##";
    bufferevent_write(bev,SendBuffer,strlen(SendBuffer));

}

void event_cb(struct bufferevent *bev, short events, char*arg)
{
    if(events & BEV_EVENT_EOF)
    {
        printf("connnection closed!\n");
    }
    else if(events & BEV_EVENT_ERROR)
    {
        printf("some other error\n");
    }
    stCallback *callback=(struct stCallback*)arg;
    free(callback);
    callback=NULL;
    printf("指针释放\n");
    bufferevent_free(bev);

}


void listen_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr *addr,int len, void *ptr)
{
    printf("new connection!\n");
    struct event_base* base = (struct event_base*)ptr;
    struct bufferevent* bev = NULL;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE); //创建一个bufferevent事件
    stCallback *callback =(stCallback *) malloc(sizeof (stCallback));
    callback->m_nRecvedSize =0;
    callback->m_RecvdataSize=0;
    memset(callback, 0, sizeof (stCallback));
    bufferevent_setcb(bev,read_cb, write_cb, event_cb, callback);  //设置回调函数
    bufferevent_enable(bev, EV_READ);//将读事件加入到监听列表
    bufferevent_enable(bev, EV_WRITE);
}


int main(int argc, char *argv[])
{
   struct event_base* base = event_base_new();

   // init server info
   struct sockaddr_in serv;
   memset(&serv, 0, sizeof(serv));
   serv.sin_family = AF_INET;
   serv.sin_port = htons(21030);
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
