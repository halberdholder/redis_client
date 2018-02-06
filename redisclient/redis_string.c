

#include <hiredis.h>

#include "redis_client.h"
#include "redis_string.h"


int redis_string_nop(redis_client *this, int index)
{
    return REDIS_OK;
}



int redis_string_init(redis_string *String)
{
	String->NOP = redis_string_nop;

    return REDIS_OK;
}

void redis_string_deinit(redis_string *String)
{
    ;
}


