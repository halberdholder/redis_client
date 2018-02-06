
#include <string.h>
#include <errno.h>
#include <hiredis.h>

#include "_redis_client.h"
#include "redis_client.h"
#include "redis_set.h"


static int _redis_set_sadd_p(redis_client *this, int index, const char *key, const char *member)
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

        rc = redisAppendCommand(this->redis, "SADD %s %s", key, member);
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

        rc = redisAppendCommand(this->redis, "SADD %s %s", key, member);
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

static int _redis_set_sadd_s(redis_client *this, int index, const char *key, const char *member)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "SADD %s %s", key, member);

    rc = _redis_command_status(this, cmd);

    return rc;
}

static int _redis_set_srem_p(redis_client *this, int index, const char *key, const char *member)
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

        rc = redisAppendCommand(this->redis, "SREM %s %s", key, member);
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

        rc = redisAppendCommand(this->redis, "SREM %s %s", key, member);
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

static int _redis_set_srem_s(redis_client *this, int index, const char *key, const char *member)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "SREM %s %s", key, member);

    rc = _redis_command_status(this, cmd);

    return rc;
}


/**
 * @param
 * va_list : members, last member must be NULL.
 */
int redis_set_sadd(redis_client *this, int index, const char *key, const char *member)
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
        rc = _redis_set_sadd_p(this, index, key, member);
    }
    else
    {
        rc = _redis_set_sadd_s(this, index, key, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}


/**
 * @param
 * va_list : members, last member must be NULL.
 */
int redis_set_srem(redis_client *this, int index, const char *key, const char *member)
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
        rc = _redis_set_srem_p(this, index, key, member);
    }
    else
    {
        rc = _redis_set_srem_s(this, index, key, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_set_sismember(redis_client *this, int index, const char *key, const char *member)
{
    int rc = REDIS_FALSE;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_FALSE;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: Set.SISMEMBER don't support pipeline mode\n", __FUNCTION__);
        rc = REDIS_FALSE;
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        rc = REDIS_FALSE;
        goto on_ret;
    }

    snprintf(cmd, sizeof(cmd), "SISMEMBER %s %s", key, member);

    rc = _redis_command_int(this, cmd);
    rc = (0 != rc && -1 != rc) ? REDIS_TRUE : REDIS_FALSE;

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_set_smembers(redis_client *this, int index, const char *key, redis_member **o_members)
{
    int rc = 0;
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
        EMI_LOG("%s: Set.SMEMBERS don't support pipeline mode\n", __FUNCTION__);
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

    snprintf(cmd, sizeof(cmd), "SMEMBERS %s", key);

    rc = _redis_command_strings(this, cmd, REDIS_FALSE, o_members);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_set_sscan(redis_client *this, int index, 
                          const char *key, const char *pattern, int count, 
                          redis_member **o_members)
{
    int rc = 0;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || 
        !pattern || '\0' == pattern[0] || !o_members)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return -1;
    }

    *o_members = NULL;

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: Set.SSCAN don't support pipeline mode\n", __FUNCTION__);
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

    snprintf(cmd, sizeof(cmd), "SSCAN %s 0 MATCH %s COUNT %d", key, pattern, count);

    rc = _redis_command_strings(this, cmd, REDIS_TRUE, o_members);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_set_init(redis_set *Set)
{
	Set->SADD      = redis_set_sadd;
	Set->SREM      = redis_set_srem;
	Set->SISMEMBER = redis_set_sismember;
    Set->SMEMBERS  = redis_set_smembers;
    Set->SSCAN     = redis_set_sscan;

    return REDIS_OK;
}

void redis_set_deinit(redis_set *Set)
{
    ;
}

