
#ifndef __REDIS_TYPES_H
#define __REDIS_TYPES_H


struct __redis_member;
typedef struct __redis_member redis_member;
struct __redis_score_member;
typedef struct __redis_score_member redis_score_member;
struct __redis_client;
typedef struct __redis_client redis_client;
struct __redis_key;
typedef struct __redis_key redis_key;
struct __redis_string;
typedef struct __redis_string redis_string;
struct __redis_set;
typedef struct __redis_set redis_set;
struct __redis_sortedset;
typedef struct __redis_sortedset redis_sortedset;
struct __redis_list;
typedef struct __redis_list redis_list;
struct __redis_hash;
typedef struct __redis_hash redis_hash;

#if 0
#include "redis_key.h"
#include "redis_string.h"
#include "redis_set.h"
#include "redis_sortedset.h"
#include "redis_list.h"
#include "redis_hash.h"
#endif

//#include "redis_client.h"

#endif
