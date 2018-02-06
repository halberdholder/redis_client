
#ifndef __REDIS_HASH_H
#define __REDIS_HASH_H


#include "redis_types.h"
#include "redis_hash_desc.h"


typedef struct __redis_hash
{
    int   (*HSET)(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, const void *data, const char *member);
    int   (*HSET2)(redis_client *this, int index, const char *key, const char *member, const char *value);
    int   (*HMSET)(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, const void *data, ...);
    int   (*HSETALL)(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, const void *data);
    int   (*HGET)(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, void *data, const char *member);
    char* (*HGET2)(redis_client *this, int index, const char *key, const char *member);
    int   (*HMGET)(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, void *data, ...);
    int   (*HGETALL)(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, void *data);
    int   (*HDEL)(redis_client *this, int index, const char *key, const char *member);
    int   (*HEXISTS)(redis_client *this, int index, const char *key, const char *member);
    int   (*HINCRBY)(redis_client *this, int index, const char *key, const char *member, int increment);

} redis_hash;


int redis_hash_init(redis_hash *Hash);
void redis_hash_deinit(redis_hash *Hash);


#endif


