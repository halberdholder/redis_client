
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hiredis.h>

#include "redis_client.h"
#include "_redis_client.h"
#include "redis_sortedset.h"


static int 
_redis_sortedset_zadd_p(redis_client *this, int index, const char *key, int score, const char *member)
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

        rc = redisAppendCommand(this->redis, "ZADD %s %d %s", key, score, member);
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

        rc = redisAppendCommand(this->redis, "ZADD %s %d %s", key, score, member);
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
_redis_sortedset_zadd_s(redis_client *this, int index, const char *key, int score, const char *member)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "ZADD %s %d %s", key, score, member);

    rc = _redis_command_status(this, cmd);

    return rc;
}

static int 
_redis_sortedset_zincrby_p(redis_client *this, int index, const char *key, int score, const char *member)
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

        rc = redisAppendCommand(this->redis, "ZINCRBY %s %d %s", key, score, member);
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

        rc = redisAppendCommand(this->redis, "ZINCRBY %s %d %s", key, score, member);
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
_redis_sortedset_zincrby_s(redis_client *this, int index, const char *key, int score, const char *member)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "ZINCRBY %s %d %s", key, score, member);

    rc = _redis_command_status(this, cmd);

    return rc;
}

static int 
_redis_sortedset_zrem_p(redis_client *this, int index, const char *key, const char *member)
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

        rc = redisAppendCommand(this->redis, "ZREM %s %s", key, member);
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

        rc = redisAppendCommand(this->redis, "ZADD %s %s", key, member);
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
_redis_sortedset_zrem_s(redis_client *this, int index, const char *key, const char *member)
{
    int rc = REDIS_OK;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        return rc;
    }

    snprintf(cmd, sizeof(cmd), "ZREM %s %s", key, member);

    rc = _redis_command_status(this, cmd);

    return rc;
}


int redis_sortedset_zadd(redis_client *this, int index, const char *key, int score, const char *member)
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
        rc = _redis_sortedset_zadd_p(this, index, key, score, member);
    }
    else
    {
        rc = _redis_sortedset_zadd_s(this, index, key, score, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_sortedset_zcount(redis_client *this, int index, const char *key, int min_score, int max_score)
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
        EMI_LOG("%s: SortedSet.ZCOUNT don't support pipeline mode\n", __FUNCTION__);
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

    snprintf(cmd, sizeof(cmd), "ZCOUNT %s %d %d", key, min_score, max_score);

    rc = _redis_command_int(this, cmd);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_sortedset_zincrby(redis_client *this, int index, const char *key, int score, const char *member)
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
        rc = _redis_sortedset_zincrby_p(this, index, key, score, member);
    }
    else
    {
        rc = _redis_sortedset_zincrby_s(this, index, key, score, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}

/**
 * @param
 * withscores
 * - REDIS_TRUE : WITHSCORES
 * - REDIS_FALSE: WITH OUT SCORES
 * 
 * o_data
 * if WITHSCORES, o_data type must be redis_member**, 
 * other wise redis_score_member**.
 */
int redis_sortedset_zrange(redis_client *this, int index, 
                                     const char *key, int start, int stop, int withscores, 
                                     void **o_data)
{
    int rc = -1;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !o_data)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return -1;
    }

    *o_data = NULL;

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: SortedSet.ZRANGE don't support pipeline mode\n", __FUNCTION__);
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

    if (REDIS_TRUE == withscores)
    {
        snprintf(cmd, sizeof(cmd), "ZRANGE %s %d %d WITHSCORES", key, start, stop);
        rc = _redis_command_score_strings(this, cmd, REDIS_FALSE, (redis_score_member **)o_data);
    }
    else
    {
        snprintf(cmd, sizeof(cmd), "ZRANGE %s %d %d", key, start, stop);
        rc = _redis_command_strings(this, cmd, REDIS_FALSE, (redis_member **)o_data);
    }

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

/**
 * @param
 * withscores
 * - REDIS_TRUE : WITHSCORES
 * - REDIS_FALSE: WITH OUT SCORES
 * 
 * o_data
 * if WITHSCORES, o_data type must be redis_member**, 
 * other wise redis_score_member**.
 */
int redis_sortedset_zrangebyscore(redis_client *this, int index, 
                                     const char *key, int min, int max, int withscores, 
                                     void **o_data)
{
    int rc = -1;
    char min_b[12] = {0};
    char max_b[12] = {0};
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !o_data)
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return -1;
    }

    *o_data = NULL;

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: SortedSet.ZRANGEBYSCORE don't support pipeline mode\n", __FUNCTION__);
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

    min <= INT_MIN ? snprintf(min_b, sizeof(min_b), "-inf") : snprintf(min_b, sizeof(min_b), "%d", min);
    max >= INT_MAX ? snprintf(max_b, sizeof(max_b), "+inf") : snprintf(max_b, sizeof(max_b), "%d", max);

    if (REDIS_TRUE == withscores)
    {
        snprintf(cmd, sizeof(cmd), "ZRANGEBYSCORE %s %s %s WITHSCORES", key, min_b, max_b);
        rc = _redis_command_score_strings(this, cmd, REDIS_FALSE, (redis_score_member **)o_data);
    }
    else
    {
        snprintf(cmd, sizeof(cmd), "ZRANGEBYSCORE %s %s %s", key, min_b, min_b);
        rc = _redis_command_strings(this, cmd, REDIS_FALSE, (redis_member **)o_data);
    }

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_sortedset_zscore(redis_client *this, int index, const char *key, const char *member)
{
    int rc = INT_MAX;
    char *score = NULL;
    char cmd[MAX_SINGLE_CMD_LEN] = {0};

    if (!this || index < 0 || !key || '\0' == key[0] || !member || '\0' == member[0])
    {
        EMI_LOG("%s: invalid parameter\n", __FUNCTION__);
        return INT_MAX;
    }

    pthread_mutex_lock(&this->lock);

    if (this->pipeline >= 0)
    {
        EMI_LOG("%s: SortedSet.ZSCORE don't support pipeline mode\n", __FUNCTION__);
        rc = INT_MAX;
        goto on_ret;
    }

    rc = _redis_try_connect_nonblock(this, index);
    if (REDIS_OK != rc)
    {
        EMI_LOG("%s: _redis_try_connect_nonblock failed\n", __FUNCTION__);
        rc = INT_MAX;
        goto on_ret;
    }

    snprintf(cmd, sizeof(cmd), "ZSCORE %s %s", key, member);

    score = _redis_command_string(this, cmd);
    rc = (!score || '\0' == score[0]) ? INT_MAX : atoi(score);
    free(score);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_sortedset_zscan(redis_client *this, int index, 
                                   const char *key, const char *pattern, int count, 
                                   redis_score_member **o_members)
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
        EMI_LOG("%s: SortedSet.ZSCAN don't support pipeline mode\n", __FUNCTION__);
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

    snprintf(cmd, sizeof(cmd), "ZSCAN %s 0 MATCH %s COUNT %d", key, pattern, count);

    rc = _redis_command_score_strings(this, cmd, REDIS_TRUE, o_members);

on_ret:
    pthread_mutex_unlock(&this->lock);

    return rc;
}

int redis_sortedset_zrem(redis_client *this, int index, const char *key, const char *member)
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
        rc = _redis_sortedset_zrem_p(this, index, key, member);
    }
    else
    {
        rc = _redis_sortedset_zrem_s(this, index, key, member);
    }

    pthread_mutex_unlock(&this->lock);

    return rc;
}


int redis_sortedset_init(redis_sortedset *SortedSet)
{
	SortedSet->ZADD          = redis_sortedset_zadd;
	SortedSet->ZCOUNT        = redis_sortedset_zcount;
	SortedSet->ZINCRBY       = redis_sortedset_zincrby;
    SortedSet->ZRANGE        = redis_sortedset_zrange;
    SortedSet->ZRANGEBYSCORE = redis_sortedset_zrangebyscore;
    SortedSet->ZSCORE        = redis_sortedset_zscore;
    SortedSet->ZSCAN         = redis_sortedset_zscan;
    SortedSet->ZREM          = redis_sortedset_zrem;

    return REDIS_OK;
}

void redis_sortedset_deinit(redis_sortedset *SortedSet)
{
    ;
}

