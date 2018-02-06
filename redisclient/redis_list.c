
#include <string.h>
#include <errno.h>
#include <hiredis.h>

#include "redis_client.h"
#include "_redis_client.h"
#include "redis_list.h"


static int 
_redis_list_push_p(redis_client *this, int index, int left, const char *key, const char *member)
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

        if (REDIS_TRUE == left)
        {
            rc = redisAppendCommand(this->redis, "LPUSH %s %s", key, member);
        }
        else
        {
            rc = redisAppendCommand(this->redis, "RPUSH %s %s", key, member);
        }

        if (REDIS_OK != rc)
        {
            EMI_LOG("%s: pipeline mode, redisAppendCommand error: %s\n", __FUNCTION__, 
                     REDIS_ERR_IO == this->redis->err ? strerror(errno) : this->redis->errstr);
            return rc;
        }
    }
    else
    {
        if (this->db_index != index)
        {
            EMI_LOG("%s: pipeline mode, can't change database index from %d to %d\n", 
                     __FUNCTION__, this->db_index, index);
            return REDIS_ERR;
        }

        if (REDIS_TRUE == left)
        {
            rc = redisAppendCommand(this->redis, "LPUSH %s %s", key, member);
        }
        else
        {
            rc = redisAppendCommand(this->redis, "RPUSH %s %s", key, member);
        }

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

static int 
_redis_list_push_s(redis_client *this, int index, int left, const char *key, const char *member)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    if (REDIS_TRUE == left)
    {
        snprintf(cmd, sizeof(cmd), "LPUSH %s %s", key, member);
    }
    else
    {
        snprintf(cmd, sizeof(cmd), "RPUSH %s %s", key, member);
    }

    rc = _redis_command_status(this, cmd);

    return rc;
}

static int 
_redis_list_rem_p(redis_client *this, int index, const char *key, int count, const char *member)
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

        rc = redisAppendCommand(this->redis, "LREM %s %d %s", key, count, member);

        if (REDIS_OK != rc)
        {
            EMI_LOG("%s: pipeline mode, redisAppendCommand error: %s\n", __FUNCTION__, 
                     REDIS_ERR_IO == this->redis->err ? strerror(errno) : this->redis->errstr);
            return rc;
        }
    }
    else
    {
        if (this->db_index != index)
        {
            EMI_LOG("%s: pipeline mode, can't change database index from %d to %d\n", 
                     __FUNCTION__, this->db_index, index);
            return REDIS_ERR;
        }

        rc = redisAppendCommand(this->redis, "LREM %s %d %s", key, count, member);

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

static int 
_redis_list_rem_s(redis_client *this, int index, const char *key, int count, const char *member)
{
    int rc = -1;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "LREM %s %d %s", key, count, member);

    rc = _redis_command_int(this, cmd);

    return rc;
}


int redis_list_lpush(redis_client *this, int index, const char *key, const char *member)
{
    int rc = REDIS_OK;

    if (!this || index < 0 || !key || '\0' == key[0] || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_list_push_p(this, index, REDIS_TRUE, key, member);
    }
    else
    {
        rc = _redis_list_push_s(this, index, REDIS_TRUE, key, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_list_rpush(redis_client *this, int index, const char *key, const char *member)
{
    int rc = REDIS_OK;

    if (!this || index < 0 || !key || '\0' == key[0] || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_list_push_p(this, index, REDIS_FALSE, key, member);
    }
    else
    {
        rc = _redis_list_push_s(this, index, REDIS_FALSE, key, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

char *redis_list_lpop(redis_client *this, int index, const char *key)
{
    int rc = REDIS_OK;
    char *member = NULL;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return NULL;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: List.LPOP don't support pipeline mode\n", __FUNCTION__);
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        goto on_ret;
    }

    snprintf(cmd, sizeof(cmd), "LPOP %s", key);

    member = _redis_command_string(this, cmd);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return member;
}

char *redis_list_rpop(redis_client *this, int index, const char *key)
{
    int rc = REDIS_OK;
    char *member = NULL;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return NULL;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: List.RPOP don't support pipeline mode\n", __FUNCTION__);
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        goto on_ret;
    }

    snprintf(cmd, sizeof(cmd), "RPOP %s", key);

    member = _redis_command_string(this, cmd);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return member;
}

char *redis_list_blpop(redis_client *this, int index, const char *key)
{
    char *member = NULL;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return NULL;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: List.LPOP don't support pipeline mode\n", __FUNCTION__);
        goto on_ret;
    }

    _redis_try_connect_block(this, index);

    snprintf(cmd, sizeof(cmd), "BLPOP %s", key);

    member = _redis_command_string(this, cmd);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return member;
}

char *redis_list_brpop(redis_client *this, int index, const char *key)
{
    char *member = NULL;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return NULL;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: List.RPOP don't support pipeline mode\n", __FUNCTION__);
        goto on_ret;
    }

    _redis_try_connect_block(this, index);

    snprintf(cmd, sizeof(cmd), "BRPOP %s", key);

    member = _redis_command_string(this, cmd);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return member;
}

int redis_list_llen(redis_client *this, int index, const char *key)
{
    int rc = -1;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return -1;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: List.LLEN don't support pipeline mode\n", __FUNCTION__);
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        rc = -1;
        goto on_ret;
    }

    snprintf(cmd, sizeof(cmd), "LLEN %s", key);

    rc = _redis_command_int(this, cmd);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_list_lrange(redis_client *this, int index, const char *key, int start, int stop, redis_member **o_members)
{
    int rc = -1;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !o_members)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return -1;
    }

    *o_members = NULL;

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: List.LRANGE don't support pipeline mode\n", __FUNCTION__);
        rc = -1;
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        rc = -1;
        goto on_ret;
    }

    snprintf(cmd, sizeof(cmd), "LRANGE %s %d %d", key, start, stop);
    rc = _redis_command_strings(this, cmd, REDIS_FALSE, o_members);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_list_lrem(redis_client *this, int index, const char *key, int count, const char *member)
{
    int rc = REDIS_ERR;

    if (!this || index < 0 || !key || '\0' == key[0] || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_list_rem_p(this, index, key, count, member);
    }
    else
    {
        rc = _redis_list_rem_s(this, index, key, count, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}


int redis_list_init(redis_list *List)
{
    List->LPUSH  = redis_list_lpush;
    List->RPUSH  = redis_list_rpush;
    List->LPOP   = redis_list_lpop;
    List->RPOP   = redis_list_rpop;
    List->BLPOP  = redis_list_blpop;
    List->BRPOP  = redis_list_brpop;
    List->LLEN   = redis_list_llen;
    List->LRANGE = redis_list_lrange;
    List->LREM   = redis_list_lrem;

    return REDIS_OK;
}

void redis_list_deinit(redis_list *List)
{
    ;
}

