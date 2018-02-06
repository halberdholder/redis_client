
#ifndef __REDIS_CLIENT_H
#define __REDIS_CLIENT_H

#include <limits.h>
#include <pthread.h>
#include <hiredis.h>

#if 1
#include "redis_types.h"
//#else
#include "redis_key.h"
#include "redis_string.h"
#include "redis_hash.h"
#include "redis_list.h"
#include "redis_set.h"
#include "redis_sortedset.h"
#endif

#ifndef EMI_LOG
#define EMI_LOG printf
#endif


#define MAX_SINGLE_CMD_LEN  1024
#define MAX_MEMBER_LEN      256

#define REDIS_TRUE  1
#define REDIS_FALSE 0

#ifndef INT_MAX
#define INT_MAX ((~0) >> 1)
#endif

#ifndef INT_MIN
#define INT_MIN (~((~0) >> 1))
#endif


struct __redis_client
{
    char                ip[16];                 /* Server IP */
    int                 port;                   /* Server Port */
    redisContext       *redis;                  /* hiredis context */
    int                 db_index;               /* Indicate database index in hiredis context */

    pthread_mutex_t     lock;

    /**
     * Currently, in pipeline mode ? 
     * - INT_MIN: single command mode
     * - >= 0   : pipeline mode, and indicate currently pipeline command count
     */
    int                 pipeline;

    /**
     * Enter pipeline mode
     */
    int                 (*pipeline_create)(struct __redis_client *);

    /**
     * Send all commands in pipeline to server, and exit pipeline mode
     */
    int                 (*pipeline_exec)(struct __redis_client *);

    /**
     * Change database index, work well in pipeline mode and single command mode
     */
    int                 (*SELECT)(struct __redis_client *, int);

    redis_key           Key;
    redis_string        String;
    redis_hash          Hash;
    redis_list          List;
    redis_set           Set;
    redis_sortedset     SortedSet;
};

struct __redis_member
{
    char member[MAX_MEMBER_LEN];
};

struct __redis_score_member
{
    int  score;
    char member[MAX_MEMBER_LEN];
};


redis_client *redis_client_create(const char *ip, int port);
void redis_client_destroy(redis_client *redis_db);


#endif

