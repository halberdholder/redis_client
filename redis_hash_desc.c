
#include <sys/types.h>
#include <stdlib.h>
#include "redis_hash_desc.h"

#include "demo.h"


#ifndef sizeof2
#define sizeof2(TYPE, MEMBER)   (size_t)(sizeof(((TYPE *)0)->MEMBER))
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER)  ((size_t)&(((TYPE *)0)->MEMBER))
#endif


redis_hash_member hdesc_tbls_account[] = 
{
    { "id",           REDIS_INT,     sizeof(int),                       offsetof(redis_account, id) },
    { "username",     REDIS_STR,     sizeof2(redis_account, username),  offsetof(redis_account, username)},
    { "password",     REDIS_STR,     sizeof2(redis_account, password),  offsetof(redis_account, password)},
    { "vip",          REDIS_INT,     sizeof(int),                       offsetof(redis_account, vip)},
    { NULL, 0, 0, 0},
};


