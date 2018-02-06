
#ifndef ____REDIS_CLIENT_H
#define ____REDIS_CLIENT_H


#include "redis_types.h"


int _redis_try_connect_nonblock(redis_client *rds_client, int index);
void _redis_try_connect_block(redis_client *rds_client, int index);


int _redis_command_status(redis_client *c, const char *cmd);
int _redis_command_int(redis_client *c, const char *cmd);
char *_redis_command_string(redis_client *c, const char *cmd);
int _redis_command_strings(redis_client *c, const char *cmd, int scan_flag, redis_member **o_members);
int _redis_command_score_strings(redis_client *c, const char *cmd, int scan_flag, redis_score_member **o_members);


#endif

