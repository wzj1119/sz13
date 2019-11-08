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
#include <time.h>
#include <QString>
#include <QSettings>
#include <QVariant>
#include <QFile>
#include <QTextCodec>
#include "libbson-1.0/bson.h"
#include "libmongoc-1.0/mongoc.h"
#include "mongodb.h"
#include "log.h"
#include "pthread.h"

typedef unsigned char byte;

#define SavePSize               256
#define MAX_MSG_LEN             1024 * 1024*10
#define SavedataSize            1024 * 1024*5
#define RECVSIZE                1024 * 1024

QThreadPool pool;


QString httpUploadurl;
QString httpUpload;
int serverPort;

int count=0;
int number=0;

typedef struct _stCallback{
     uint8_t          cRecvBuff[RECVSIZE]; //循环接收数据
     uint8_t          m_RecvBuffer[MAX_MSG_LEN];//处理黏包
     int              m_nRecvedSize;      //接收到的数据总大小
     int              m_RecvdataSize;    //数据体总大小
     char             Savedata[SavedataSize]; //存数据体
     char             pName[SavePSize]; //存名称
     char             Savesim[20];       //存sim号
     char             Saveeventtype[6]; //存事件类型
     char             Saveurl[96]; //存url（路径及文件名）
     char             Savemediatype[6]; //存文件类型
     char             Savemediaformat[6];//文件类型
     char             Savecurltime[68];     //存上传http所用时间
     char             Saveffmpegtime[68];   //存压缩视频所用时间
     char             Savecollectiontime[68];   //存开始到结束的时间
     char             Saverecvtime[68];       //存接收完整数据所用时间
     char             Savefiletime[68];   //存保存文件所用时间
     char             SaveMongoDBtime[68];  //操作数据库所用时间
}stCallback;
char SavehttpUpload[30];

bool loadConfig()
{

    //解析配置文件
    QString strSetting=QCoreApplication::applicationDirPath()+"/conf/gateway.ini";
    qDebug() << strSetting;


    QFile file(strSetting);
    if(!file.exists())
    {
        qDebug("配置文件未找到");
        return false;

    }
    QSettings settings(strSetting,QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    serverPort=settings.value("Server/ServerPort").toInt();
    httpUploadurl=settings.value("HttpServer/httpUploadurl").toString();
    httpUpload=settings.value("HttpServer/httpUpload").toString();
    QByteArray httpload;

    httpload.append(httpUpload);
    char *httpSaveAddress=httpload.data();

    memset(SavehttpUpload,0,30);
    memcpy(SavehttpUpload,httpSaveAddress,strlen(httpSaveAddress));

    return true;

}
//生成文件
int save15sFile( char *buffer,int len, char* h15sFileName)
{
    FILE *fp=NULL;
    fp = fopen(h15sFileName,"ab+");
    if(fp==NULL)
    {
        qDebug("文件未找到");
        return -1;
    }
    fwrite(buffer,1,len,fp);
    fclose(fp);
    return 0;
}

//视频压缩
int ffmpeg(char*str,char * fileId)
{
     QString strSetting=QCoreApplication::applicationDirPath()+"/file/";
     QString strffm=QCoreApplication::applicationDirPath()+"/compressfileName/";
     QString arr= str;
     QString des=fileId;

     QString filePath = strSetting + arr + fileId+ ".mp4";
     QString ffmfilePath=strffm + arr + fileId + ".mp4";
     QString execStr = "ffmpeg -i " + filePath + " -r 10 -b:v 320k -b:a 32k -strict -2 -y " + ffmfilePath;

    FILE *fp=popen(execStr.toStdString().c_str(),"r"); //建立管道
    if(!fp)
    {
        return -1;
    }
    pclose(fp);
    return 0;

}
//图像上传到http
int curlpecture(char*sim,char*fileId)
{
    int time=3;
    QString Setcurl=QCoreApplication::applicationDirPath()+"/file/";
    QString str=sim;
    QString des=fileId;
    QString curlPath=Setcurl +str + fileId + ".jpg";
    QString httpPath=httpUploadurl + "/media/fu?f=" + str + fileId + ".jpg";

    QString curlStr="curl -H Expect: -F file=@" + curlPath + " " + httpPath;

    QFile file(Setcurl);
    if(!file.exists())
    {
        qDebug("文件未找到");
        return -1;
    }
    FILE*fpl=NULL;
    while(time>0)
    {
        fpl=popen(curlStr.toStdString().c_str(),"w");
        if(fpl!=NULL)
        {
            pclose(fpl);
            break;
        }
        time--;
        pclose(fpl);
    }
    if(fpl==NULL)
    {
       qDebug("文件上传失败");
       return -1;
    }
   // pclose(fpl);
    number++;
    qDebug()<<"上传成功数:"<<number;
   return 0;
}
//视频上传到http
int curlvedio(char*sim,char* fileId)
{
    int time=3;

    QString Setcurl=QCoreApplication::applicationDirPath()+"/compressfileName/";
    QString str=sim;
    QString des=fileId;

    QString curlPath=Setcurl + str + fileId + ".mp4";
    QString httpPath=httpUploadurl  + "/media/fu?f=" + str + fileId + ".mp4";

    QString curlStr="curl -H Expect: -F file=@" + curlPath + " " +  httpPath;

    QFile file(Setcurl);
    if(!file.exists())
    {
        qDebug("文件未找到");
        return -1;

    }

    FILE*fpl=NULL;
    while(time>0)
    {
      fpl=popen(curlStr.toStdString().c_str(),"w");
      if(fpl!=NULL)
      {
          pclose(fpl);
          break;
      }
      time--;
      pclose(fpl);
    }
    if(fpl==NULL)
    {
        qDebug("文件上传失败");
        return -1;
    }
   // pclose(fpl);
    number++;
    qDebug()<<"上传成功数:"<<number;
   return 0;
}

//读回调
void read_cb(struct bufferevent *bev, void*arg)
{
    pool.setMaxThreadCount(3);
    pthread * run=new pthread;


    int starttime=clock();
    int recvlen = 0;  //每次收到数据的长度
    stCallback *callback=(stCallback*)arg;

    // read data from cache
    memset(callback->cRecvBuff, 0, RECVSIZE);
    recvlen = bufferevent_read(bev, callback->cRecvBuff, RECVSIZE); //接收数据
    int recvstarttime=clock();
    if(recvlen > 0)
    {
        // copy data from cRecvBuff to m_RecvBuffer
       // printf("recv DataSize:%d\n",recvlen);
        memcpy(callback->m_RecvBuffer+callback->m_nRecvedSize, callback->cRecvBuff, recvlen); //将缓存中的数据存如m_RecvBuffer中，黏包处理
        callback->m_nRecvedSize += recvlen;  //接收到的数据总大小为每次收到数据之和
    }

    // 数据处理
   while(1)
  {
       int len=0;
       int res=0;
       if(callback->m_nRecvedSize < 25)   //根据协议包头不足25，退出
       {
           break;
       }
       if(callback->m_RecvBuffer[len]!=0x40 && callback->m_RecvBuffer[len+1]!=0x40)  //判断m_RecvBuff中前两个字节是否为头标识（##）
       {
          printf("error!\n");
          qDebug("未检测到头标识...");
          break;
       }
       int IdIndex=len+2;  //文件ID起始字节
       int Id=(callback->m_RecvBuffer[IdIndex]<<24| callback->m_RecvBuffer[IdIndex+1]<<16 |callback->m_RecvBuffer[IdIndex+2]<<8 | callback->m_RecvBuffer[IdIndex+3]);  //文件ID
       int btype=callback->m_RecvBuffer[len+6];//文件类型
       int eventType=callback->m_RecvBuffer[len+7];//事件类型
       memset(callback->Savesim,0,20);
       sprintf(callback->Savesim,"%x%02x%02x%02x%02x%02x",callback->m_RecvBuffer[len+9],callback->m_RecvBuffer[len+10],callback->m_RecvBuffer[len+11],callback->m_RecvBuffer[len+12],callback->m_RecvBuffer[len+13],callback->m_RecvBuffer[len+14]);

       int datalenIndex = len + 19;   //数据体长度起始字节
       //数据体长度
       int datalen=(callback->m_RecvBuffer[datalenIndex]<<24 | callback->m_RecvBuffer[datalenIndex+1]<<16 | callback->m_RecvBuffer[datalenIndex+2]<<8 | callback->m_RecvBuffer[datalenIndex+3]);

       int dataIndex=datalenIndex+4;  //数据体内容起始字节
      // printf("len:%d\n",datalen);

       if(25+datalen>callback->m_nRecvedSize)
       {
           break;
       }

       if(datalen>0)
       {
           memcpy(callback->Savedata+callback->m_RecvdataSize,callback->m_RecvBuffer+dataIndex,datalen); //当datalen>0时，将每包数据的数据体内容不断的存到Savedata中
           callback->m_RecvdataSize+=datalen;  //数据体总大小为每次收到数据体长度之和
        }

      if(datalen==0) //当收到的数据中数据体长度为0时，把数据存成文件并存入MongoDB
      {

           qDebug("end of file,发送结束标志到终端设备");
           qDebug(callback->Savesim);
           struct evbuffer *sendbuf = evbuffer_new();
           byte msgHeader[2] = {'@', '@'};
           byte status = 0;
           byte msgTail[2] = {'#', '#'};
           evbuffer_add(sendbuf, msgHeader, 2);
           evbuffer_add(sendbuf, &status, 1);
           evbuffer_add(sendbuf, msgTail, 2);

           bufferevent_write_buffer(bev, sendbuf);  //发送消息到终端

           evbuffer_free(sendbuf);

           int mongo=0;
           int recvendtime=clock();
           memset(callback->Saverecvtime,0,68);
           sprintf(callback->Saverecvtime,"接收完整数据所用%dms",recvendtime-recvstarttime);
           qDebug(callback->Saverecvtime);


           memset(callback->Saveeventtype,0,6);
           sprintf(callback->Saveeventtype,"%d",eventType);  //事件类型转换为字符串

            memset(callback->Saveurl,0,96);
            memset(callback->Savefiletime,0,68);
            memset(callback->Savecurltime,0,68);
            memset(callback->SaveMongoDBtime,0,68);

           memset(callback->Savemediatype,0,6);
           sprintf(callback->Savemediatype,"%d",btype); //文件类型转换为字符串

           memset(callback->Savemediaformat,0,6);
           sprintf(callback->Savemediaformat,"%d",btype==0?btype:6);   //文件类型不为0则为6
           if(2==eventType) //开关门图像
           {
                memset(callback->pName,0,SavePSize);

                sprintf(callback->pName,"file/%s%d.jpg",callback->Savesim,Id);  //图片路径及名称


                int Pfilestarttime=clock();
                res=save15sFile(callback->Savedata,callback->m_RecvdataSize,callback->pName); //调用save15sFile
                if(res!=0)
                {
                    qDebug() << callback->pName << "图像存入本地失败...";
                }
                if(0==res)
                {
                    qDebug() << callback->pName << "图像存入本地成功...";
                }
                qDebug()<<"图像名称"<<callback->pName;

                int Pfileendtime=clock();
                sprintf(callback->Savefiletime,"存入图像文件所用%dms",Pfileendtime-Pfilestarttime);
                qDebug(callback->Savefiletime);

                memset(callback->pName,0,SavePSize);
                sprintf(callback->pName,"%d",Id);  //文件ID转换为字符串

                sprintf(callback->Saveurl,"http://%s%s%d.jpg",SavehttpUpload,callback->Savesim,Id);  //url路径及图像文件名称

                int monstarttime=clock();
                mongo=MongoDB(callback->pName,callback->Savesim,callback->Saveeventtype,callback->Saveurl,callback->Savemediatype,callback->Savemediaformat);  //将数据存入MongoDB
                if(mongo!=0)
                {
                    qDebug()<< "图像数据存入MongoDB失败...";

                }
                int monendtime=clock();
                sprintf(callback->SaveMongoDBtime,"操作MongoDB所用%dms",monendtime-monstarttime);
                qDebug()<<callback->SaveMongoDBtime;

                int curlstarttime=clock();
                res=curlpecture(callback->Savesim,callback->pName);
                if(res!=0)
                {
                    qDebug()<< "图像上传http失败";

                }
                int curlendtime=clock();

                sprintf(callback->Savecurltime,"图片上传到http所用%dms",curlendtime-curlstarttime);
                qDebug(callback->Savecurltime);

           }
            if(0==eventType || 1==eventType) //0：举报视频 1：紧急视频
            {

                memset(callback->pName,0,SavePSize);

                sprintf(callback->pName,"file/%s%d.mp4",callback->Savesim,Id);  //视频存储路径及名称

                int Vfilestarttime=clock();
                res=save15sFile(callback->Savedata,callback->m_RecvdataSize,callback->pName);//调用save15sFile
                if(res!=0)
                {

                    qDebug() << callback->pName << "视频存入本地失败...";

                }
                if(0==res)
                {
                     qDebug() << callback->pName << "视频存入本地成功...";

                }
                qDebug()<<"视频文件名称"<<callback->pName;

                int Vfileendtiem=clock();
                sprintf(callback->Savefiletime,"存入视频文件所用%dms",Vfileendtiem-Vfilestarttime);
                qDebug(callback->Savefiletime);


                memset(callback->pName,0,SavePSize);
                sprintf(callback->pName,"%d",Id);
                int ffmpegstarttime=clock();
                res=ffmpeg(callback->Savesim,callback->pName);
                if(res!=0)
                {
                    qDebug("ffmpeg 压缩失败...");
                }
                if(0==res)
                {
                    qDebug("ffmpeg压缩成功...");
                    int ffmpegendtime=clock();
                    memset(callback->Saveffmpegtime,0,68);
                    sprintf(callback->Saveffmpegtime,"压缩视频所用%dms",ffmpegendtime-ffmpegstarttime);
                    qDebug(callback->Saveffmpegtime);
                }

                sprintf(callback->Saveurl,"http://%s%s%d.mp4",SavehttpUpload,callback->Savesim,Id); //url路径及视频文件名称
                int mongostarttime=clock();
                mongo=MongoDB(callback->pName,callback->Savesim,callback->Saveeventtype,callback->Saveurl,callback->Savemediatype,callback->Savemediaformat);  //将数据存入MongoDB
                if(mongo!=0)
                {
                    qDebug()<<"视频数据存入MongoDB失败";
                }
                int mongoendtime=clock();
                sprintf(callback->SaveMongoDBtime,"操作MongoDB所用%dms",mongoendtime-mongostarttime);
                qDebug()<<callback->SaveMongoDBtime;

                int curlvediostarttime=clock();
                res=curlvedio(callback->Savesim,callback->pName);
                if(res!=0)
                {
                    qDebug() << "视频上传http失败";
                }
                int curlvedioendtime=clock();
                sprintf(callback->Savecurltime,"视频上传到http所用%dms",curlvedioendtime-curlvediostarttime);
                qDebug(callback->Savecurltime);

            }
             int endtime=clock();
             memset(callback->Savecollectiontime,0,68);
             sprintf(callback->Savecollectiontime,"从开始到结束全部所用%dms",endtime-starttime);
             qDebug(callback->Savecollectiontime);
             callback->m_RecvdataSize=0;  //每次将数据存成文件后数据体总和清零
      }
        callback->m_nRecvedSize -=(25+datalen);   //每包数据解析后，接收到的数据总长度减去此包长度
        memcpy(callback->m_RecvBuffer, callback->m_RecvBuffer+25 + datalen, callback->m_nRecvedSize); //将解析完的每包数据取出
    }
}

void write_cb(struct bufferevent *bev, void*arg)
{
   qDebug("write_cb callback success....");
}

void event_cb(struct bufferevent *bev, short events, void*arg)
{
    if(events & BEV_EVENT_EOF)
    {
        qDebug("connection closed.....");
    }
    else if(events & BEV_EVENT_ERROR)
    {
        qDebug("some other error %s\n",evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()));
    }
    stCallback *callback=(stCallback*)arg;
    free(callback);
    callback=NULL;
    qDebug()<<"指针释放";
    bufferevent_free(bev);
}

void listen_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr *addr,int len, void *ptr)
{
    qDebug("new collection success..");
    count++;
    qDebug()<<"连接数："<<count;
    struct event_base* base = (struct event_base*)ptr;
    struct bufferevent* bev = NULL;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE); //创建一个bufferevent事件
    stCallback *callback =(stCallback *) malloc(sizeof (stCallback));
    callback->m_nRecvedSize =0;
    callback->m_RecvdataSize=0;
    memset(callback, 0, sizeof (stCallback));

    bufferevent_setwatermark(bev,EV_WRITE,100,200);

    bufferevent_setcb(bev,read_cb, NULL, event_cb, callback);  //设置回调函数
    
    bufferevent_enable(bev, EV_READ);//将读写事件加入到监听列表

}

int main(int argc, char *argv[])
{
   QCoreApplication a(argc, argv);
   qInstallMessageHandler(outputMessage);

   struct event_base* base = event_base_new();
   loadConfig();

   // init server info
   struct sockaddr_in serv;
   memset(&serv, 0, sizeof(serv));
   serv.sin_family = AF_INET;
   serv.sin_port = htons(serverPort);
   serv.sin_addr.s_addr = htonl(INADDR_ANY);

   struct evconnlistener* listen = NULL;
   listen = evconnlistener_new_bind(base, listen_cb, base,
                                    LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                    -1, (struct sockaddr*)&serv, sizeof(serv));

   qDebug("sever start success...");


   // loop
   event_base_dispatch(base);
   //release resource
   evconnlistener_free(listen);
   event_base_free(base);

   return a.exec();
}
