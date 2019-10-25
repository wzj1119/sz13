#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/time.h>
#include "libbson-1.0/bson.h"
#include "libmongoc-1.0/mongoc.h"

void gettime();

int MongoDB(char*idfile,char*simId,char*eventtype,char*url,char*mediatype,char*mediaformat);
