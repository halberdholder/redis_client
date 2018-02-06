
#ifndef __REDIS_HASH_DESC_H
#define __REDIS_HASH_DESC_H


typedef enum 
{
    REDIS_INT = 0, 
    REDIS_STR = 1,

} dtype;

typedef struct __redis_hash_member
{
    const char   *member;
    dtype         data_type;
    int           data_size;
    int           offset;

} redis_hash_member;

#endif
