
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <hiredis.h>

#include "redis_client.h"
#include "_redis_client.h"


/**
 * When lost connection to server, 
 * the default interval to conenct to server is 1(sec)
 */
#define REDIS_TRY_CONNECT_INTERVAL  1


/**
 * when rds_client->redis == NULL call this function
 */
static int __redis_connect(redis_client *rds_client)
{
    rds_client->redis = redisConnect(rds_client->ip, rds_client->port);
    if (!rds_client->redis)
    {
        EMI_LOG("%s: failed on redisConnect[%s:%d]\n", __FUNCTION__, rds_client->ip, rds_client->port);

        return REDIS_ERR;
    }
    else if (0 != rds_client->redis->err)
    {
        EMI_LOG("%s: failed on redisConnect[%s:%d]: %s\n", __FUNCTION__, 
                       rds_client->ip, rds_client->port, rds_client->redis->errstr);
        redisFree(rds_client->redis);
        rds_client->redis = NULL;
        return REDIS_ERR;
    }

    return REDIS_OK;
}

/**
 * when rds_client->redis == NULL or rds_client->db_idx != index call this function 
 */
static int __redis_select(redis_client *rds_client, int index)
{
    redisReply *reply = NULL;
    char cmd[64] = {0};

    snprintf(cmd, sizeof(cmd), "SELECT %d", index);

    reply = (redisReply *)redisCommand(rds_client->redis, cmd);

	if (!reply)
	{
        EMI_LOG("%s: lost connection to redis server\n", __FUNCTION__);

        redisFree(rds_client->redis);
        rds_client->redis = NULL;

        return REDIS_ERR;
    }
    else if (REDIS_REPLY_ERROR == reply->type)
    {
        EMI_LOG("%s: failed on redisCommand[SELECT %d]: %s\n", 
                   __FUNCTION__, index, reply->str ? reply->str : NULL);

        freeReplyObject(reply);

        return REDIS_ERR;
    }
    else
    {
        rds_client->db_index = index;

        freeReplyObject(reply);
    }

    return REDIS_OK;
}

static int __redis_parse_reply(redisReply *reply, redis_member **o_members)
{
    int i = 0;
    redisReply *sub_reply = NULL;

    switch (reply->type)
    {
        case REDIS_REPLY_ARRAY:
            *o_members = (redis_member *)malloc(sizeof(redis_member) * reply->elements);
            if (!(*o_members))
            {
                EMI_LOG("%s: FATAL, out of memory\n", __FUNCTION__);
                return -1;
            }

            memset(*o_members, 0, sizeof(redis_member) * reply->elements);

            for (i = 0; i < reply->elements; ++i)
            {
                sub_reply = reply->element[i];
                switch (sub_reply->type) 
                {
                    case REDIS_REPLY_INTEGER:
                        snprintf((*o_members)[i].member, MAX_MEMBER_LEN, "%lld", sub_reply->integer);
                        break;
                    case REDIS_REPLY_STRING:
                        if (0 != strncmp("nil", sub_reply->str, sub_reply->len))
                        {
                            snprintf((*o_members)[i].member, MAX_MEMBER_LEN, "%.*s", sub_reply->len, sub_reply->str);
                        }
                        else
                        {
                            (*o_members)[i].member[0] = '\0';
                        }
                        break;
                    case REDIS_REPLY_NIL:
                        (*o_members)[i].member[0] = '\0';
                        break;
                    default:
                        EMI_LOG("%s: FATAL, got sub reply type[%d]\n", __FUNCTION__, sub_reply->type);
                        free(*o_members);
                        return -1;
                }
            }

            return reply->elements;

        case REDIS_REPLY_STRING:
            /* don't break */
        case REDIS_REPLY_INTEGER:
            *o_members = (redis_member *)malloc(sizeof(redis_member));
            if (!(*o_members))
            {
                EMI_LOG("%s: FATAL, out of memory\n", __FUNCTION__);
                return -1;
            }

            memset(*o_members, 0, sizeof(redis_member));

            if (REDIS_REPLY_INTEGER == reply->type)
            {
                snprintf((*o_members)->member, MAX_MEMBER_LEN, "%lld", reply->integer);
            }
            else
            {
                if (0 == strncmp("nil", reply->str, reply->len))
                {
                    (*o_members)->member[0] = '\0';
                }
                else
                {
                    snprintf((*o_members)->member, MAX_MEMBER_LEN, "%.*s", reply->len, reply->str);
                }
            }

            return 1;

        case REDIS_REPLY_NIL:
            return 0;

        default:
            EMI_LOG("%s: FATAL, got reply type[%d]\n", __FUNCTION__, reply->type);
            break;
    }

    return -1;
}


int _redis_try_connect_nonblock(redis_client *rds_client, int index)
{
    if (!rds_client->redis)
    {
        if (REDIS_OK != __redis_connect(rds_client))
        {
            return REDIS_ERR;
        }

        if (REDIS_OK != __redis_select(rds_client, index))
        {
            return REDIS_ERR;
        }
    }

    if (rds_client->db_index != index)
    {
        if (REDIS_OK != __redis_select(rds_client, index))
        {
            return REDIS_ERR;
        }
    }

    return REDIS_OK;
}

void _redis_try_connect_block(redis_client *rds_client, int index)
{
    while (REDIS_OK != _redis_try_connect_nonblock(rds_client, index))
    {
        sleep(REDIS_TRY_CONNECT_INTERVAL);
    }
}

/**
 * @return status
 * - REDIS_OK : command execute success
 * - REDIS_ERR: command execute failed
 */
int _redis_command_status(redis_client *c, const char *cmd)
{
    redisReply *reply = NULL;

    EMI_LOG("%s: cmd[%s]\n", __FUNCTION__, cmd);

    reply = (redisReply *)redisCommand(c->redis, cmd);
    if (!reply)
    {
        EMI_LOG("%s: redisCommand error: %s\n", __FUNCTION__, 
                 REDIS_ERR_IO == c->redis->err ? strerror(errno) : c->redis->errstr);

        redisFree(c->redis);
        c->redis = NULL;
        c->db_index = -1;

        return REDIS_ERR;
    }

    if (REDIS_REPLY_ERROR == reply->type)
    {
        EMI_LOG("%s: redisCommand reply error: %s\n", 
                   __FUNCTION__, reply->str ? reply->str : NULL);

        freeReplyObject(reply);

        return REDIS_ERR;
    }

    EMI_LOG("%s: redisCommand success\n", __FUNCTION__);

    freeReplyObject(reply);

    return REDIS_OK;
}

/**
 * @return count
 * -  >= 0 : count
 * -  <  0 : command failed
 */
int _redis_command_int(redis_client *c, const char *cmd)
{
    int rc = 0;
    redisReply *reply = NULL;

    EMI_LOG("%s: cmd[%s]\n", __FUNCTION__, cmd);

    reply = (redisReply *)redisCommand(c->redis, cmd);
    if (!reply)
    {
        EMI_LOG("%s: redisCommand error: %s\n", __FUNCTION__, 
                 REDIS_ERR_IO == c->redis->err ? strerror(errno) : c->redis->errstr);

        redisFree(c->redis);
        c->redis = NULL;
        c->db_index = -1;

        return -1;
    }

    if (REDIS_REPLY_ERROR == reply->type)
    {
        EMI_LOG("%s: redisCommand reply error: %s\n", 
                   __FUNCTION__, reply->str ? reply->str : NULL);

        freeReplyObject(reply);

        return -1;
    }

    EMI_LOG("%s: redisCommand success\n", __FUNCTION__);

    if (REDIS_REPLY_INTEGER == reply->type)
    {
        rc = reply->integer;
    }
    else
    {
        rc = -1;
    }

    freeReplyObject(reply);

    return rc;
}

/**
 * @return a string
 * -  NULL: empty string or command failed
 */
char *_redis_command_string(redis_client *c, const char *cmd)
{
    char *str = NULL;
    redisReply *reply = NULL;

    EMI_LOG("%s: cmd[%s]\n", __FUNCTION__, cmd);

    reply = (redisReply *)redisCommand(c->redis, cmd);
    if (!reply)
    {
        EMI_LOG("%s: redisCommand error: %s\n", __FUNCTION__, 
                 REDIS_ERR_IO == c->redis->err ? strerror(errno) : c->redis->errstr);

        redisFree(c->redis);
        c->redis = NULL;
        c->db_index = -1;

        return NULL;
    }

    if (REDIS_REPLY_ERROR == reply->type)
    {
        EMI_LOG("%s: redisCommand reply error: %s\n", 
                   __FUNCTION__, reply->str ? reply->str : NULL);

        freeReplyObject(reply);

        return NULL;
    }

    EMI_LOG("%s: redisCommand success\n", __FUNCTION__);

    if (REDIS_REPLY_STRING != reply->type)
    {
        EMI_LOG("%s: redisCommand reply type: %d\n", __FUNCTION__, reply->type);
        goto on_ret;
    }

    str = malloc(reply->len + 1);
    if (!str)
    {
        EMI_LOG("%s: FATAL, out of memory\n", __FUNCTION__);
        goto on_ret;
    }

    if (0 == strncmp("nil", reply->str, reply->len))
    {
        str[0] = '\0';
    }
    else
    {
        snprintf(str, reply->len + 1, "%.*s", reply->len, reply->str);
    }

on_ret:
    freeReplyObject(reply);

    return str;
}

/**
 * @return a few strings and the counts
 * -  <= 0: no data or command failed
 * -  >  0: counts of strings
 *
 * @param
 * o_members: out data, strings array
 */
int _redis_command_strings(redis_client *c, const char *cmd, int scan_flag, redis_member **o_members)
{
    int count = 0;
    redisReply *reply = NULL;

    EMI_LOG("%s: cmd[%s]\n", __FUNCTION__, cmd);

    reply = (redisReply *)redisCommand(c->redis, cmd);
    if (!reply)
    {
        EMI_LOG("%s: redisCommand error: %s\n", __FUNCTION__, 
                 REDIS_ERR_IO == c->redis->err ? strerror(errno) : c->redis->errstr);

        redisFree(c->redis);
        c->redis = NULL;
        c->db_index = -1;

        return -1;
    }

    if (REDIS_REPLY_ERROR == reply->type)
    {
        EMI_LOG("%s: redisCommand reply error: %s\n", 
                   __FUNCTION__, reply->str ? reply->str : NULL);

        freeReplyObject(reply);

        return -1;
    }

    EMI_LOG("%s: redisCommand success\n", __FUNCTION__);

    if (REDIS_FALSE == scan_flag)
    {
        count = __redis_parse_reply(reply, o_members);
    }
    else
    {
        if (REDIS_REPLY_ARRAY != reply->type || 2 != reply->elements)
        {
            EMI_LOG("%s: *scan reply error: reply type[%d], reply elements[%ld]\n", 
                     __FUNCTION__, reply->type, reply->elements);
            freeReplyObject(reply);
            return -1;
        }

        redisReply *sub_reply = reply->element[1];
        if (REDIS_REPLY_ERROR == sub_reply->type)
        {
            EMI_LOG("%s: *scan sub reply error: %s\n", 
                       __FUNCTION__, sub_reply->str ? sub_reply->str : NULL);
            freeReplyObject(reply);
            return -1;
        }

        count = __redis_parse_reply(sub_reply, o_members);
    }

    freeReplyObject(reply);

    return count;
}

/**
 * @return a few strings with score and the counts
 * -  <= 0: no data or command failed
 * -  >  0: counts of strings
 *
 * @param
 * o_members: out data, strings array with score
 */
int _redis_command_score_strings(redis_client *c, const char *cmd, int scan_flag, redis_score_member **o_members)
{
    int i = 0, count = 0;
    redisReply *reply = NULL;
    redis_member *members = NULL;

    EMI_LOG("%s: cmd[%s]\n", __FUNCTION__, cmd);

    reply = (redisReply *)redisCommand(c->redis, cmd);
    if (!reply)
    {
        EMI_LOG("%s: redisCommand error: %s\n", __FUNCTION__, 
                 REDIS_ERR_IO == c->redis->err ? strerror(errno) : c->redis->errstr);

        redisFree(c->redis);
        c->redis = NULL;
        c->db_index = -1;

        return -1;
    }

    if (REDIS_REPLY_ERROR == reply->type)
    {
        EMI_LOG("%s: redisCommand reply error: %s\n", 
                   __FUNCTION__, reply->str ? reply->str : NULL);

        freeReplyObject(reply);

        return -1;
    }

    EMI_LOG("%s: redisCommand success\n", __FUNCTION__);

    if (REDIS_FALSE == scan_flag)
    {
        count = __redis_parse_reply(reply, &members);
    }
    else
    {
        if (REDIS_REPLY_ARRAY != reply->type || 2 != reply->elements)
        {
            EMI_LOG("%s: *scan reply error: reply type[%d], reply elements[%ld]\n", 
                     __FUNCTION__, reply->type, reply->elements);
            freeReplyObject(reply);
            return -1;
        }

        redisReply *sub_reply = reply->element[1];
        if (REDIS_REPLY_ERROR == sub_reply->type)
        {
            EMI_LOG("%s: *scan sub reply error: %s\n", 
                       __FUNCTION__, sub_reply->str ? sub_reply->str : NULL);
            freeReplyObject(reply);
            return -1;
        }

        count = __redis_parse_reply(sub_reply, &members);
    }

    if (count > 0)
    {
        /* check count is dual number? */
        if (count & 1)
        {
            EMI_LOG("%s: FATAL, reply count must be dual number\n", __FUNCTION__);
            count = -1;
            goto on_ret;
        }

        count /= 2;
        *o_members = (redis_score_member *)malloc(sizeof(redis_score_member) * count);
        if (!(*o_members))
        {
            EMI_LOG("%s: FATAL, out of memory\n", __FUNCTION__);
            count = -1;
            goto on_ret;
        }

        memset(*o_members, 0, sizeof(redis_score_member) * count);

        for (i = 0; i < count; ++i)
        {
            (*o_members)[i].score = atoi(members[2*i + 1].member);
            snprintf((*o_members)[i].member, MAX_MEMBER_LEN, "%s", members[2*i].member);
        }
    }

on_ret:
    if (members)
    {
        free(members);
    }

    freeReplyObject(reply);

    return count;
}



