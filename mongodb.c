#include "mongodb.h"


/*char buf[24]; //存id_media
char Saveid_key[36];  //存id_key
void gettime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    long timeuse=tv.tv_sec*1000+tv.tv_usec/1000;  //当前时间毫秒值（时间戳）
    long time=tv.tv_sec;  //当前时间（秒）
    memset(buf,0,24);
    sprintf(buf,"%ld",timeuse);
    sprintf(Saveid_key,"%d%d",getpid(),time);

}*/
//void MongoDB(char *idfile,char*eventtype,char*url,char*mediatype,char*mediaformat)
int MongoDB(char*arg)
{
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
   // gettime();
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
    cursor=mongoc_collection_find_with_opts(tempDBcollection,query,&opts,NULL);

    while(mongoc_cursor_next(cursor,&doc))
    {
        str=bson_as_json(doc,NULL);
       // printf("str:%s\n",str);
        bson=bson_new_from_json((const uint8_t *)str,-1,&error);
        if(!bson)
        {
                fprintf(stderr,"%s\n",error.message);

        }
        string=bson_as_json(bson,NULL);
      /*  BSON_APPEND_UTF8(bson,"event_type",eventtype);
        BSON_APPEND_UTF8(bson,"id_key",Saveid_key);
        BSON_APPEND_UTF8(bson,"url",url);
        BSON_APPEND_UTF8(bson,"media_type",mediatype);
        BSON_APPEND_UTF8(bson,"media_format",mediaformat);
        BSON_APPEND_UTF8(bson,"id_media",buf);*/
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
