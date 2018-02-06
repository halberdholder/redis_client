
#ifndef __REDIS_KEY_H
#define __REDIS_KEY_H


#include "redis_types.h"


struct __redis_key
{
    int (*DEL)(redis_client *this, int index, const char *key);
    int (*EXISTS)(redis_client *this, int index, const char *key);
    int (*EXPIRE)(redis_client *this, int index, const char *key, unsigned seconds);

};


int redis_key_init(redis_key *Key);
void redis_key_deinit(redis_key *Key);


#endif

