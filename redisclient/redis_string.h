
#ifndef __REDIS_STRING_H
#define __REDIS_STRING_H


#include "redis_types.h"


typedef struct __redis_string
{
    int (*NOP)(redis_client *this, int index);

} redis_string;


int redis_string_init(redis_string *String);
void redis_string_deinit(redis_string *String);


#endif

