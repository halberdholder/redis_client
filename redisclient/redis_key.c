
#include <string.h>
#include <errno.h>
#include <hiredis.h>

#include "_redis_client.h"
#include "redis_client.h"
#include "redis_key.h"


static int _redis_key_del_p(redis_client *this, int index, const char *key)
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

        rc = redisAppendCommand(this->redis, "DEL %s", key);
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

        rc = redisAppendCommand(this->redis, "DEL %s", key);
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

static int _redis_key_del_s(redis_client *this, int index, const char *key)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "DEL %s", key);

    rc = _redis_command_status(this, cmd);

    return rc;
}

static int _redis_key_expire_p(redis_client *this, int index, const char *key, unsigned seconds)
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

        rc = redisAppendCommand(this->redis, "EXPIRE %s %u", key, seconds);
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

        rc = redisAppendCommand(this->redis, "EXPIRE %s %u", key, seconds);
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

static int _redis_key_expire_s(redis_client *this, int index, const char *key, unsigned seconds)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "EXPIRE %s %u", key, seconds);

    rc = _redis_command_status(this, cmd);

    return rc;
}


int redis_key_del(redis_client *this, int index, const char *key)
{
    int rc = REDIS_OK;

    if (!this || index < 0 || !key || '\0' == key[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_key_del_p(this, index, key);
    }
    else
    {
        rc = _redis_key_del_s(this, index, key);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

/**
 * @return
 * -  REDIS_TRUE:  exists
 * -  REDIS_FALSE: not exists or exec failed
 */
int redis_key_exists(redis_client *this, int index, const char *key)
{
    int rc = REDIS_FALSE;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_FALSE;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: Key.EXISTS don't support pipeline mode\n", __FUNCTION__);
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

    snprintf(cmd, sizeof(cmd), "EXISTS %s", key);

    rc = _redis_command_int(this, cmd);
    rc = (0 != rc && -1 != rc) ? REDIS_TRUE : REDIS_FALSE;

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}


int redis_key_expire(redis_client *this, int index, const char *key, unsigned seconds)
{
    int rc = REDIS_OK;

    if (!this || index < 0 || !key || '\0' == key[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_key_expire_p(this, index, key, seconds);
    }
    else
    {
        rc = _redis_key_expire_s(this, index, key, seconds);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}


int redis_key_init(redis_key *Key)
{
	Key->DEL    = redis_key_del;
	Key->EXISTS = redis_key_exists;
	Key->EXPIRE = redis_key_expire;

    return REDIS_OK;
}

void redis_key_deinit(redis_key *Key)
{
    ;
}


