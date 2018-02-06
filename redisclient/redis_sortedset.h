
#ifndef __REDIS_SORTEDSET_H
#define __REDIS_SORTEDSET_H


#include "redis_types.h"


typedef struct __redis_sortedset
{
    int (*ZADD)(redis_client *this, int index, const char *key, int score, const char *member);
    int (*ZCOUNT)(redis_client *this, int index, const char *key, int min_score, int max_score);
    int (*ZINCRBY)(redis_client *this, int index, const char *key, int score, const char *member);
    int (*ZRANGE)(redis_client *this, int index, const char *key, int start, int stop, int withscores, void **o_data);
    int (*ZRANGEBYSCORE)(redis_client *this, int index, const char *key, int min, int max, int withscores, void **o_data);
    int (*ZSCORE)(redis_client *this, int index, const char *key, const char *member);
    int (*ZSCAN)(redis_client *this, int index, const char *key, const char *pattern, int count, redis_score_member **o_members);
    int (*ZREM)(redis_client *this, int index, const char *key, const char *member);

} redis_sortedset;


int redis_sortedset_init(redis_sortedset *SortedSet);
void redis_sortedset_deinit(redis_sortedset *SortedSet);


#endif

