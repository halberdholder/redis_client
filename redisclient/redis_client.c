
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "redis_types.h"
#include "_redis_client.h"
#include "redis_client.h"


static int _redis_select_p(redis_client *this, int index)
{
    int rc = REDIS_OK;

    if (0 == this->pipeline)
    {
        rc = _redis_try_connect_nonblock(this, index);
        if (REDIS_OK != rc)
        {
            EMI_LOG("%s: pipeline mode, _redis_try_connect_nonblock failed\n", __FUNCTION__);
            return rc;
        }
    }
    else
    {
        if (this->db_index == index)
        {
            return REDIS_OK;
        }

        rc = redisAppendCommand(this->redis, "SELECT %d", index);
        if (REDIS_OK != rc)
        {
            EMI_LOG("%s: pipeline mode, redisAppendCommand error: %s\n", __FUNCTION__, 
                     REDIS_ERR_IO == this->redis->err ? strerror(errno) : this->redis->errstr);
            return rc;
        }
    }

    this->pipeline++;

    return rc;
}

static int _redis_select_s(redis_client *this, int index)
{
    int rc = REDIS_OK;

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    return rc;
}


static int redis_pipeline_create(redis_client *this)
{
    if (INT_MIN != this->pipeline)
    {
        EMI_LOG("%s: allready in pipeline mode\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    this->pipeline = 0;

    return REDIS_OK;
}

static int redis_pipeline_exec(redis_client *this)
{
    int rc = REDIS_OK;
    redisReply *reply = NULL;

    if (INT_MIN == this->pipeline)
    {
        EMI_LOG("%s: currently, not in pipeline mode\n", __FUNCTION__);
        return REDIS_ERR;
    }

    EMI_LOG("%s: %d commands in pipeline\n", __FUNCTION__, this->pipeline);

    while (this->pipeline)
    {
        rc = redisGetReply(this->redis, (void **)&reply);
        if (REDIS_OK != rc)
        {
            EMI_LOG("%s: redisGetReply error: %s\n", __FUNCTION__, 
                     REDIS_ERR_IO == this->redis->err ? strerror(errno) : this->redis->errstr);
        }

        freeReplyObject(reply);

        this->pipeline--;
    }

    /* exit pipeline mode */
    this->pipeline = INT_MIN;

    pthread_mutex_unlock(&this->lock);

    return REDIS_OK;
}

static int redis_select(redis_client *this, int index)
{
    int rc = REDIS_OK;

    if (!this || index < 0)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_select_p(this, index);
    }
    else
    {
        rc = _redis_select_s(this, index);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

redis_client *redis_client_create(const char *ip, int port)
{
    redis_client *c = NULL;

    c = (redis_client *)malloc(sizeof(redis_client));
    if (!c)
    {
        EMI_LOG("%s: out of memory, malloc rds_client failed\n", __FUNCTION__);
        return NULL;
    }

    snprintf(c->ip, sizeof(c->ip), "%s", ip);
    c->port = port;
    c->redis = NULL;
    c->db_index = -1;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&c->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    c->pipeline = INT_MIN;
    c->pipeline_create = redis_pipeline_create;
    c->pipeline_exec = redis_pipeline_exec;

    c->SELECT = redis_select;

    redis_key_init(&c->Key);
    redis_string_init(&c->String);
    redis_hash_init(&c->Hash);
    redis_list_init(&c->List);
    redis_set_init(&c->Set);
    redis_sortedset_init(&c->SortedSet);

    return c;
}

void redis_client_destroy(redis_client *this)
{
    if (this)
    {
        pthread_mutex_destroy(&this->lock);

        redis_key_deinit(&this->Key);
        redis_string_deinit(&this->String);
        redis_hash_deinit(&this->Hash);
        redis_list_deinit(&this->List);
        redis_set_deinit(&this->Set);
        redis_sortedset_deinit(&this->SortedSet);

        if (this->redis)
        {
            redisFree(this->redis);
        }

        free(this);
    }
}


