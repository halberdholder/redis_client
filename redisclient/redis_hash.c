
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hiredis.h>

#include "redis_client.h"
#include "_redis_client.h"
#include "redis_hash.h"


static int _redis_hash_set_p(redis_client *this, int index, const char *cmd)
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

        rc = redisAppendCommand(this->redis, cmd);
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

        rc = redisAppendCommand(this->redis, cmd);
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

static int _redis_hash_set_s(redis_client *this, int index, const char *cmd)
{
    int rc = REDIS_OK;

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    rc = _redis_command_status(this, cmd);

    return rc;
}

static int _redis_hash_hdel_p(redis_client *this, int index, const char *key, const char *member)
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

        rc = redisAppendCommand(this->redis, "HDEL %s %s", key, member);
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

        rc = redisAppendCommand(this->redis, "HDEL %s %s", key, member);
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

static int _redis_hash_hdel_s(redis_client *this, int index, const char *key, const char *member)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "HDEL %s %s", key, member);

    rc = _redis_command_status(this, cmd);

    return rc;
}

static int _redis_hash_hincrby_p(redis_client *this, int index, const char *key, const char *member, int increment)
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

        rc = redisAppendCommand(this->redis, "HINCRBY %s %s %d", key, member, increment);
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

        rc = redisAppendCommand(this->redis, "HINCRBY %s %s %d", key, member, increment);
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

static int _redis_hash_hincrby_s(redis_client *this, int index, const char *key, const char *member, int increment)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "HINCRBY %s %s %d", key, member, increment);

    rc = _redis_command_status(this, cmd);

    return rc;
}


/**
 * @retrun
 * REDIS_OK :  success
 * REDIS_ERR:  failed
 */
int redis_hash_hset(redis_client *this, int index, const char *key, 
                           redis_hash_member *hdesc_tbls, const void *data, const char *member)
{
    int rc = REDIS_OK;
    int i = 0;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || 
        !hdesc_tbls || !data || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    for (i = 0; hdesc_tbls[i].member; ++i)
    {
        if (0 == strcmp(hdesc_tbls[i].member, member))
        {
            break;
        }
    }

    if (!hdesc_tbls[i].member)
    {
        EMI_LOG("%s: member[%s] not found in hash desc table\n", __FUNCTION__, member);
        return REDIS_ERR;
    }

    if (REDIS_INT == hdesc_tbls[i].data_type)
    {
        snprintf(cmd, sizeof(cmd), "HSET %s %s %d", key, member, *(int *)(data + hdesc_tbls[i].offset));
    }
    else
    {
        if ('\0' == ((char *)(data + hdesc_tbls[i].offset))[0])
        {
            EMI_LOG("%s: member[%s] value is empty, do nothing\n", __FUNCTION__, member);
            return REDIS_OK;
        }
        snprintf(cmd, sizeof(cmd), "HSET %s %s %.*s", key, member, 
                                    hdesc_tbls[i].data_size, (char *)(data + hdesc_tbls[i].offset));
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_hash_set_p(this, index, cmd);
    }
    else
    {
        rc = _redis_hash_set_s(this, index, cmd);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

/**
 * @retrun
 * REDIS_OK :  success
 * REDIS_ERR:  failed
 */
int redis_hash_hset2(redis_client *this, int index, const char *key, const char *member, const char *value)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || 
        !member || '\0' == member[0] || !value || '\0' == value[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    snprintf(cmd, sizeof(cmd), "HSET %s %s %s", key, member, value);

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_hash_set_p(this, index, cmd);
    }
    else
    {
        rc = _redis_hash_set_s(this, index, cmd);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

/**
 * @param
 * va_list : members, last member must be NULL.
 */
int redis_hash_hmset(redis_client *this, int index, const char *key, 
                           redis_hash_member *hdesc_tbls, const void *data, ...)
{
    int rc = REDIS_OK;
    int i = 0, len = 0, count = 0;
    char *member = NULL;
    va_list args;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !hdesc_tbls || !data)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    len = snprintf(cmd, sizeof(cmd), "HMSET %s", key);

    va_start(args, data);
    while (1)
    {
        member = va_arg(args, char *);
        if (!member)
        {
            break;
        }

        for (i = 0; hdesc_tbls[i].member; ++i)
        {
            if (0 == strcmp(hdesc_tbls[i].member, member))
            {
                break;
            }
        }

        if (!hdesc_tbls[i].member)
        {
            EMI_LOG("%s: member[%s] not found in hash desc table\n", __FUNCTION__, member);
            return REDIS_ERR;
        }

        if (REDIS_INT == hdesc_tbls[i].data_type)
        {
            len += snprintf(cmd + len, sizeof(cmd) - len, " %s %d", member, 
                            *(int *)(data + hdesc_tbls[i].offset));
        }
        else
        {
            if ('\0' == ((char *)(data + hdesc_tbls[i].offset))[0])
            {
                EMI_LOG("%s: member[%s] value is empty\n", __FUNCTION__, member);
                continue;
            }
            len += snprintf(cmd + len, sizeof(cmd) - len, " %s %.*s", member, 
                            hdesc_tbls[i].data_size, (char *)(data + hdesc_tbls[i].offset));
        }

        count++;
    }
    va_end(args);

    if (0 == count)
    {
        EMI_LOG("%s: no member specified or all member is empty\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_hash_set_p(this, index, cmd);
    }
    else
    {
        rc = _redis_hash_set_s(this, index, cmd);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_hash_hsetall(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, const void *data)
{
    int rc = REDIS_OK;
    int i = 0, len = 0, count = 0;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !hdesc_tbls || !data)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    len = snprintf(cmd, sizeof(cmd), "HMSET %s", key);

    for (i = 0; hdesc_tbls[i].member; ++i)
    {
        if (REDIS_INT == hdesc_tbls[i].data_type)
        {
            len += snprintf(cmd + len, sizeof(cmd) - len, " %s %d", hdesc_tbls[i].member, 
                            *(int *)(data + hdesc_tbls[i].offset));
        }
        else
        {
            if ('\0' == ((char *)(data + hdesc_tbls[i].offset))[0])
            {
                EMI_LOG("%s: member[%s] value is empty\n", __FUNCTION__, hdesc_tbls[i].member);
                continue;
            }
            len += snprintf(cmd + len, sizeof(cmd) - len, " %s %.*s", hdesc_tbls[i].member, 
                            hdesc_tbls[i].data_size, (char *)(data + hdesc_tbls[i].offset));
        }

        count++;
    }

    if (0 == count)
    {
        EMI_LOG("%s: no member specified or all member is empty\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_hash_set_p(this, index, cmd);
    }
    else
    {
        rc = _redis_hash_set_s(this, index, cmd);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_hash_hget(redis_client *this, int index, const char *key, 
                           redis_hash_member *hdesc_tbls, void *data, const char *member)
{
    int rc = REDIS_OK;
    int i = 0;
    char *value = NULL;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || 
        !hdesc_tbls || !data || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    for (i = 0; hdesc_tbls[i].member; ++i)
    {
        if (0 == strcmp(hdesc_tbls[i].member, member))
        {
            break;
        }
    }

    if (!hdesc_tbls[i].member)
    {
        EMI_LOG("%s: member[%s] not found in hash desc table\n", __FUNCTION__, member);
        return REDIS_ERR;
    }

    memset(data + hdesc_tbls[i].offset, 0, hdesc_tbls[i].data_size);

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: Hash.HGET don't support pipeline mode\n", __FUNCTION__);
        rc = REDIS_ERR;
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        rc = REDIS_ERR;
        goto on_ret;
    }

    snprintf(cmd, sizeof(cmd), "HGET %s %s", key, member);

    value = _redis_command_string(this, cmd);
    if (!value)
    {
        rc = REDIS_ERR;
        goto on_ret;
    }

    if (REDIS_INT == hdesc_tbls[i].data_type)
    {
        *(int *)(data + hdesc_tbls[i].offset) = atoi(value);
    }
    else
    {
        snprintf((char *)(data + hdesc_tbls[i].offset), hdesc_tbls[i].data_size, "%s", value);
    }

    free(value);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

char *redis_hash_hget2(redis_client *this, int index, const char *key, const char *member)
{
    int rc = REDIS_OK;
    char *value = NULL;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return NULL;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: Hash.HGET don't support pipeline mode\n", __FUNCTION__);
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        goto on_ret;
    }

    snprintf(cmd, sizeof(cmd), "HGET %s %s", key, member);

    value = _redis_command_string(this, cmd);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return value;
}

/**
 * @param
 * va_list : members, last member must be NULL.
 */
int redis_hash_hmget(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, void *data, ...)
{
    int rc = REDIS_OK;
    int i = 0, len = 0, count = 0;
    char *member = NULL;
    redis_member *redis_members = NULL;
    va_list args;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !hdesc_tbls || !data)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    len = snprintf(cmd, sizeof(cmd), "HMGET %s", key);

    va_start(args, data);
    while (1)
    {
        member = va_arg(args, char *);
        if (!member)
        {
            break;
        }

        for (i = 0; hdesc_tbls[i].member; ++i)
        {
            if (0 == strcmp(hdesc_tbls[i].member, member))
            {
                break;
            }
        }

        if (!hdesc_tbls[i].member)
        {
            EMI_LOG("%s: member[%s] not found in hash desc table\n", __FUNCTION__, member);
            return REDIS_ERR;
        }

        memset(data + hdesc_tbls[i].offset, 0, hdesc_tbls[i].data_size);

        len += snprintf(cmd + len, sizeof(cmd) - len, " %s", member);

        count ++;
    }
    va_end(args);

    if (0 == count)
    {
        EMI_LOG("%s: no member specified\n", __FUNCTION__);
        return REDIS_ERR;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: Hash.HMGET don't support pipeline mode\n", __FUNCTION__);
        rc = REDIS_ERR;
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        rc = REDIS_ERR;
        goto on_ret;
    }

    rc = _redis_command_strings(this, cmd, REDIS_FALSE, &redis_members);
    if (rc <= 0)
    {
        rc = REDIS_ERR;
        goto on_ret;
    }

    if (rc != count)
    {
        EMI_LOG("%s: UNEXPECT, expect count[%d], got redis_member count[%d]\n", 
                 __FUNCTION__, count, rc);
        free(redis_members);
        rc = REDIS_ERR;
        goto on_ret;
    }

    va_start(args, data);
    for (count = 0, i = 0; count < rc; ++count)
    {
        member = va_arg(args, char *);

        for (; hdesc_tbls[i].member; ++i)
        {
            if (0 == strcmp(hdesc_tbls[i].member, member))
            {
                break;
            }
        }

        if (REDIS_INT == hdesc_tbls[i].data_type)
        {
            *(int *)(data + hdesc_tbls[i].offset) = atoi(redis_members[count].member);
        }
        else
        {
            snprintf((char *)(data + hdesc_tbls[i].offset), hdesc_tbls[i].data_size, "%s", redis_members[count].member);
        }
    }
    va_end(args);

    free(redis_members);
    rc = REDIS_OK;

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_hash_hgetall(redis_client *this, int index, const char *key, redis_hash_member *hdesc_tbls, void *data)
{
    int rc = REDIS_OK;
    int i = 0, len = 0;
    int data_size = 0;
    redis_member *redis_members = NULL;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !hdesc_tbls || !data)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_ERR;
    }

    len = snprintf(cmd, sizeof(cmd), "HMGET %s", key);

    for (i = 0; hdesc_tbls[i].member; ++i)
    {
        data_size += hdesc_tbls[i].data_size;

        len += snprintf(cmd + len, sizeof(cmd) - len, " %s", hdesc_tbls[i].member);
    }

    if (0 == i)
    {
        EMI_LOG("%s: no member in hash desc table\n", __FUNCTION__);
        return REDIS_ERR;
    }

    memset(data, 0, data_size);

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: Hash.HGETALL don't support pipeline mode\n", __FUNCTION__);
        rc = REDIS_ERR;
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        rc = REDIS_ERR;
        goto on_ret;
    }

    rc = _redis_command_strings(this, cmd, REDIS_FALSE, &redis_members);
    if (rc <= 0)
    {
        rc = REDIS_ERR;
        goto on_ret;
    }

    if (rc != i)
    {
        EMI_LOG("%s: UNEXPECT, expect count[%d], got redis_member count[%d]\n", 
                 __FUNCTION__, i, rc);
        free(redis_members);
        rc = REDIS_ERR;
        goto on_ret;
    }

    for (i = 0; hdesc_tbls[i].member; ++i)
    {
        if (REDIS_INT == hdesc_tbls[i].data_type)
        {
            *(int *)(data + hdesc_tbls[i].offset) = atoi(redis_members[i].member);
        }
        else
        {
            snprintf((char *)(data + hdesc_tbls[i].offset), hdesc_tbls[i].data_size, "%s", redis_members[i].member);
        }
    }

    free(redis_members);
    rc = REDIS_OK;

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_hash_hdel(redis_client *this, int index, const char *key, const char *member)
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
        rc = _redis_hash_hdel_p(this, index, key, member);
    }
    else
    {
        rc = _redis_hash_hdel_s(this, index, key, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_hash_hexists(redis_client *this, int index, const char *key, const char *member)
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
        EMI_LOG("%s: Hash.HEXISTS don't support pipeline mode\n", __FUNCTION__);
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

    snprintf(cmd, sizeof(cmd), "HEXISTS %s %s", key, member);

    rc = _redis_command_int(this, cmd);
    rc = (0 != rc && -1 != rc) ? REDIS_TRUE : REDIS_FALSE;

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_hash_hincrby(redis_client *this, int index, const char *key, const char *member, int increment)
{
    int rc = REDIS_FALSE;

    if (!this || index < 0 || !key || '\0' == key[0] || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return REDIS_FALSE;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        rc = _redis_hash_hincrby_p(this, index, key, member, increment);
    }
    else
    {
        rc = _redis_hash_hincrby_s(this, index, key, member, increment);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}


int redis_hash_init(redis_hash *Hash)
{
    Hash->HSET    = redis_hash_hset;
    Hash->HSET2   = redis_hash_hset2;
    Hash->HMSET   = redis_hash_hmset;
    Hash->HSETALL = redis_hash_hsetall;
    Hash->HGET    = redis_hash_hget;
    Hash->HGET2   = redis_hash_hget2;
    Hash->HMGET   = redis_hash_hmget;
    Hash->HGETALL = redis_hash_hgetall;
    Hash->HDEL    = redis_hash_hdel;
    Hash->HEXISTS = redis_hash_hexists;
    Hash->HINCRBY = redis_hash_hincrby;

    return REDIS_OK;
}

void redis_hash_deinit(redis_hash *Hash)
{
    ;
}


