#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<event.h>

#define PORT 21030
#define BACKLOG 100
#define SIZE 1024*1024
#define MYSIZE 1024*1024*10

struct event_base*base;
struct sock_ev
{
	struct event*read_ev;
	struct event*write_ev;
	unsigned char*buf;
	unsigned char*buffer;
	unsigned char*sendbuf;
};
void release_sock_event(struct sock_ev*ev)
{
	event_del(ev->read_ev);
	free(ev->read_ev);
	free(ev->write_ev);
	free(ev->buf);
	free(ev->buffer);
	free(ev->sendbuf);
	free(ev);
}
void on_write(int sockfd,short event,void*arg)
{
	unsigned char *sendbuf=(unsigned char*)arg;
	send(sockfd,sendbuf,6,0);
	free(sendbuf);
}
void on_read(int sockfd,short event,void*arg)
{
	struct event*write_ev;
	int recvsize;
	int m_RecivedSize=0;
	struct sock_ev*ev=(struct sock_ev*)arg;
	ev->buf=(unsigned char*)malloc(SIZE);
	ev->buffer=(unsigned char*)malloc(MYSIZE);
	ev->sendbuf=(unsigned char*)malloc(6);
	bzero(ev->buf,SIZE);
	bzero(ev->buffer,MYSIZE);
	bzero(ev->sendbuf,6);
	strcpy(ev->sendbuf,"@@0##");
	recvsize=recv(sockfd,ev->buf,SIZE,0);
	printf("recv data:%s,size:%d\n",ev->buffer,recvsize);
    //  event_set(ev->write_ev,sockfd,EV_WRITE,on_write,ev->sendbuf);
    //  event_base_set(base,ev->write_ev);
    //  event_add(ev->write_ev,NULL);
//	strcpy(ev->buffer,ev->buf);
	while(recvsize>0)
	{
		memcpy(ev->buffer,ev->buf,recvsize);
		m_RecivedSize +=recvsize;
	}
	if(recvsize==0)
	{
		release_sock_event(ev);
		close(sockfd);
		return;
	}
	for(int i=0;i<m_RecivedSize-2;++i)
	{
		
		if(ev->buffer[i]==0x40 && ev->buffer[i+1]==0x40)
		{
			int len=0;
			int n=0;
			int btype=0;
			event_set(ev->write_ev,sockfd,EV_WRITE,on_write,ev->sendbuf);
	        	event_base_set(base,ev->write_ev);
        		event_add(ev->write_ev,NULL);
			
			unsigned char startflag[128]=" ";
			strncpy(startflag,ev->buffer[i],4);
			unsigned char Id[128]=" ";
			strncpy(Id,ev->buffer[i+4],8);
			unsigned char fileType[128]=" ";
			strncpy(fileType,ev->buffer[i+12],2);
	//		uint8_t btype=ev->buffer[i+12]&0x0F;
			sscanf(fileType,"%x",&btype);
			unsigned char eventType[128]=" ";
			strncpy(eventType,ev->buffer[i+14],2);
			unsigned char cannelId[128]=" ";
			strncpy(cannelId,ev->buffer[i+16],2);
			unsigned char moId[128]=" ";
			strncpy(moId,ev->buffer[i+18],12);
			unsigned char move[128]=" ";
			strncpy(move,ev->buffer[i+30],8);
			unsigned char bytesLength[128]=" ";
			strncpy(bytesLength,ev->buffer[i+38],8);
			sscanf(bytesLength,"%x",&n);
			unsigned char*data;
			data=(unsigned char*)malloc(1024*1024);
			memset(data,0,sizeof(data));
			strncpy(data,ev->buffer[i+46],n);
			unsigned char endflag[128]=" ";
			strncpy(endflag,ev->buffer[i+46+n*2],2);
			printf("1:%s 2:%s 3:%d 4:%s 5:%s 6:%s 7:%s 8:%d 9:%s 10:%s \n",startflag,Id,btype,eventType,cannelId,moId,move,n,data,endflag);	
			if(btype==0)
			{

				FILE *fp=NULL;
				fp=fopen("file/pecture.jpg","a+");
				if(NULL==fp)
				{
					perror("fopen failed");
					return;
				}
				fprintf(fp,"%s\n",data);
			//	fwrite(data,1,strlen(data),fp);
				fclose(fp);
			}
			else if(btype==1)
			{
				FILE*fp=NULL;
				fp=fopen("file/vedio.mp4","a+");
				if(NULL==fp)
				{
					perror("fopen filed");
					return;
				}
				fprintf(fp,"%s\n",data);
			//	fwrite(data,1,strlen(data),fp);
				fclose(fp);
			}
			else
			{
				FILE*fp=NULL;
				fp=fopen("file/1file","a+");
				if(NULL==fp)
				{
					perror("fopen filed");
					return;
				}
					fprintf(fp,"%s\n",data);
				//	fwrite(data,1,strlen(data),fp);
					fclose(fp);
			}
			m_RecivedSize-=n+52;
			free(data);
		}
		if(m_RecivedSize<=i+25)
		{
			break;	
		}
	}
	
	
}
void on_accept(int sockfd,short event,void*arg)
{
	struct sockaddr_in cli_addr;
	int newfd,sin_size;
	struct sock_ev*ev=(struct sock_ev*)malloc(sizeof(struct sock_ev));
	ev->read_ev=(struct event*)malloc(sizeof(struct event));
	ev->write_ev=(struct enevt*)malloc(sizeof(struct event));
	sin_size=sizeof(struct sockaddr_in);
	newfd=accept(sockfd,(struct sockaddr*)&cli_addr,&sin_size);
	printf("accept success\n");
	event_set(ev->read_ev,newfd,EV_READ|EV_PERSIST,on_read,ev);
	event_base_set(base,ev->read_ev);
	event_add(ev->read_ev,NULL);
}
int main(int argc,char*argv[])
{
	struct sockaddr_in addr;
	int sockfd=-1;
	event_init();
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(-1==sockfd)
	{
		perror("socket");
		return 1;
	}
	int yes=1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(PORT);
	addr.sin_addr.s_addr=INADDR_ANY;
	bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
	listen(sockfd,BACKLOG);
	struct event listen_ev;
	base=event_base_new();
	event_set(&listen_ev,sockfd,EV_READ|EV_PERSIST,on_accept,NULL);
	event_base_set(base,&listen_ev);
	event_add(&listen_ev,NULL);
	event_base_dispatch(base);

	return 0;
}
