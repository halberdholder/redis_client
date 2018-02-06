
#ifndef __REDIS_SET_H
#define __REDIS_SET_H


#include "redis_types.h"


typedef struct __redis_set
{
    int (*SADD)(redis_client *this, int index, const char *key, const char *member);
    int (*SREM)(redis_client *this, int index, const char *key, const char *member);
    int (*SISMEMBER)(redis_client *this, int index, const char *key, const char *member);
    int (*SMEMBERS)(redis_client *this, int index, const char *key, redis_member **o_members);
    int (*SSCAN)(redis_client *this, int index, const char *key, const char *pattern, int count, redis_member **o_members);

} redis_set;


int redis_set_init(redis_set *Set);
void redis_set_deinit(redis_set *Set);


#endif

