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

#define MAX_MSG_LEN             1024 * 1024
#define RECVSIZE                1024 * 100

uint8_t         m_RecvBuffer[MAX_MSG_LEN];
int             m_nRecvedSize = 0;

void saveH264File(unsigned char *buffer,int len, char* h264FileName)
{
    FILE *fp;
    fp = fopen(h264FileName,"ab+");
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

//        evbuffer_add(buf, cRecvBuff, recvlen);
    }


    // sticky packet processing
    while(1)
    {
        int len = 0;
        for (len = 0; len < m_nRecvedSize-4; ++len)
        {
           if((m_RecvBuffer[len] == 0x30) && (m_RecvBuffer[len + 1] == 0x31) &&
              (m_RecvBuffer[len + 2] == 0x63) && (m_RecvBuffer[len + 3] == 0x64))
           {
               break;
           }
        }

        if(len + 4 >= m_nRecvedSize)
        {
           memset(m_RecvBuffer, 0, MAX_MSG_LEN);
           m_nRecvedSize = 0;
        }

        if(len + 32 >= m_nRecvedSize)
        {
           break;
        }

        uint8_t btype = m_RecvBuffer[len + 15] & 0xF0;
        int datalenIndex = 0;
        int datalen = 0;

        if((btype == 0x00) || (btype == 0x10) || (btype == 0x20))  // video
        {
           datalenIndex = len + 28;
        }
        else if(btype == 0x30) // audio
        {
           datalenIndex = len + 24;
        }
        else if (btype == 0x40)  // touchuan
        {
           datalenIndex = len + 16;
        }

        datalen = m_RecvBuffer[datalenIndex] * 256 + m_RecvBuffer[datalenIndex + 1];
        int videoIndex = datalenIndex + 2;
        if(videoIndex + datalen > m_nRecvedSize)
        {
           break;
        }

        if((btype == 0x00) || (btype == 0x10) || (btype == 0x20))
        {
            saveH264File(m_RecvBuffer+videoIndex, datalen, "test.h264");
        }
        else if(btype == 0x30)
        {
           // nothing
        }

        memcpy(m_RecvBuffer, m_RecvBuffer + videoIndex + datalen, m_nRecvedSize);
        m_nRecvedSize -= (videoIndex + datalen);
    }

}

void write_cb(struct bufferevent *bev, void *arg)
{
    printf("data is sent to client!\n");
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
   serv.sin_port = htons(8888);
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
