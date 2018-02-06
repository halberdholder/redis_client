
#ifndef __REDIS_LIST_H
#define __REDIS_LIST_H


#include "redis_types.h"


typedef struct __redis_list
{
    int   (*LPUSH)(redis_client *this, int index, const char *key, const char *member);
    int   (*RPUSH)(redis_client *this, int index, const char *key, const char *member);
    char* (*LPOP)(redis_client *this, int index, const char *key);
    char* (*RPOP)(redis_client *this, int index, const char *key);
    char* (*BLPOP)(redis_client *this, int index, const char *key);
    char* (*BRPOP)(redis_client *this, int index, const char *key);
    int   (*LLEN)(redis_client *this, int index, const char *key);
    int   (*LRANGE)(redis_client *this, int index, const char *key, int start, int stop, redis_member **o_members);
    int   (*LREM)(redis_client *this, int index, const char *key, int count, const char *member);

} redis_list;


int redis_list_init(redis_list *List);
void redis_list_deinit(redis_list *List);


#endif

