
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
//#define SavePSize               1024*1024*10
//#define SaveMSize               1024*1024*10
#define MAX_MSG_LEN             1024 * 1024
#define RECVSIZE                1024 * 10

uint8_t         m_RecvBuffer[MAX_MSG_LEN];
int             m_nRecvedSize = 0;
//int             m_RecvPSize=0;
//int             m_RecvMSize=0;
//uint8_t         SavePBuff[SavePSize];
//uint8_t         SaveMBuff[SaveMSize];

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

    while((recvlen = bufferevent_read(bev, cRecvBuff, RECVSIZE)) > 0)
    {
        // copy data from cRecvBuff to m_RecvBuffer
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
        uint8_t btype=m_RecvBuffer[len+6]&0xFF;
        int datalenIndex = 0;
        int datalen = 0;
        datalenIndex = len + 19;   //数据体长度起始字节
        datalen=(m_RecvBuffer[datalenIndex]<<24 | m_RecvBuffer[datalenIndex+1]<<16 | m_RecvBuffer[datalenIndex+2]<<8 | m_RecvBuffer[datalenIndex+3]);

        int dataIndex=datalenIndex+4;  //数据体内容起始字节
        if(dataIndex+datalen>m_nRecvedSize)
        {
            break;
        }
        if(btype == 0x00)
        {
             // memcpy(SavePBuff+m_RecvPSize,m_RecvBuffer+dataIndex,datalen);
             // m_RecvPSize+=datalen;
             save15sFile(m_RecvBuffer+dataIndex, datalen, "/home/wzj/worktest/15swork/file/15stest.jpg");
        }
        else if(btype==0x01)
        {
            //memcpy(SaveMBuff+m_RecvMSize,m_RecvBuffer+dataIndex,datalen);
            //m_RecvMSize+=datalen;
           save15sFile(m_RecvBuffer+dataIndex,datalen,"/home/wzj/worktest/15swork/file/15stest.mp4");
        }
        else if(btype==0x02)
        {
             //nothing
        }
        memcpy(m_RecvBuffer, m_RecvBuffer +dataIndex+ datalen, m_nRecvedSize);
        m_nRecvedSize -=(dataIndex+datalen);
    }
   //save15sFile(SavePBuff, strlen(SavePBuff),"/home/wzj/worktest/15swork/file/15stest.jpg");
   //save15sFile(SaveMBuff, strlen(SaveMBuff),"/home/wzj/worktest/15swork/file/15stest.mp4");
}

void write_cb(struct bufferevent *bev, void *arg)
{
    uint8_t SendBuffer[6];
    int sendlen=bufferevent_write(SendBuffer,"@@0##",6);
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
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    //struct evbuffer *buf = evbuffer_new();
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);
    bufferevent_enable(bev, EV_READ);
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

   printf("server start......!\n");
   // loop
   event_base_dispatch(base);
   //release resource
   evconnlistener_free(listen);
   event_base_free(base);

   return 0;
}
