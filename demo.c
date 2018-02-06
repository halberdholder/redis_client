
#include <string.h>
#include <stdlib.h>

#include "redis_client.h"
#include "demo.h"

#define PROG    "demo"


extern redis_hash_member hdesc_tbls_account[];


int main(int argc, char **argv)
{
    int i = 0, rc = 0;
    redis_client *redis_db = NULL;
    redis_member *members = NULL;
    redis_account account = {0};

    if (3 != argc)
    {
        EMI_LOG("usage: ./%s <redis_server_ip> <redis_server_port>\n\n", PROG);
        return 0;
    }

    redis_db = redis_client_create(argv[1], atoi(argv[2]));
    if (!redis_db)
    {
        EMI_LOG("%s: create redis client failed\n", __FUNCTION__);
        return 0;
    }

    EMI_LOG("===============TEST KEY================\n");
    redis_db->Set.SADD(redis_db, 0, "set", "member1");
    rc = redis_db->Key.EXISTS(redis_db, 0, "set");
    EMI_LOG("%s: key `set' %s\n", __FUNCTION__, REDIS_TRUE == rc ? "exist" : "not exist");
    redis_db->Key.EXPIRE(redis_db, 0, "set", 10);
    redis_db->Key.DEL(redis_db, 0, "set");
    EMI_LOG("\n");


    EMI_LOG("===============TEST STRING=============\n");
    rc = redis_db->String.NOP(redis_db, 0);
    EMI_LOG("%s: String.NOP %s\n", __FUNCTION__, rc == REDIS_OK ? "success" : "failed");
    EMI_LOG("\n");


    EMI_LOG("===============TEST SET================\n");
    redis_db->Set.SADD(redis_db, 0, "set", "member1");
    redis_db->Set.SADD(redis_db, 0, "set", "member2");
    rc = redis_db->Set.SISMEMBER(redis_db, 0, "set", "member1");
    EMI_LOG("%s: \"member1\" %s in set\n", __FUNCTION__, REDIS_TRUE == rc ? "is" : "is not");
    rc = redis_db->Set.SMEMBERS(redis_db, 0, "set", &members);
    EMI_LOG("Set.SMEMBERS\n");
    for (i = 0; i < rc; ++i)
    {
        EMI_LOG("\t%s\n", members[i].member);
    }
    free(members);
    members = NULL;
    rc = redis_db->Set.SSCAN(redis_db, 0, "set", "member*", INT_MAX, &members);
    EMI_LOG("Set.SSCAN\n");
    for (i = 0; i < rc; ++i)
    {
        EMI_LOG("\t%s\n", members[i].member);
    }
    free(members);
    members = NULL;
    redis_db->Set.SREM(redis_db, 0, "set", "member1");
    redis_db->Set.SREM(redis_db, 0, "set", "member2");
    EMI_LOG("\n");


    EMI_LOG("===============TEST SORTEDSET==========\n");
    redis_db->SortedSet.ZADD(redis_db, 0, "sortedset", 100, "member1");
    redis_db->SortedSet.ZADD(redis_db, 0, "sortedset", 101, "member2");
    rc = redis_db->SortedSet.ZCOUNT(redis_db, 0, "sortedset", INT_MIN, INT_MAX);
    EMI_LOG("SortedSet.ZCOUNT\n");
    EMI_LOG("\t%d\n", rc);
    redis_db->SortedSet.ZREM(redis_db, 0, "sortedset", "member1");
    redis_db->SortedSet.ZREM(redis_db, 0, "sortedset", "member2");
    EMI_LOG("TODO OTHER TEST\n");
    EMI_LOG("\n");


    EMI_LOG("===============TEST LIST===============\n");
    EMI_LOG("TODO OTHER TEST\n");
    EMI_LOG("\n");


    EMI_LOG("===============TEST HASH===============\n");
    account.id = 0;
    redis_db->Hash.HSET(redis_db, 0, "1001_00000001", hdesc_tbls_account, (void *)&account, "id");
    snprintf(account.username, sizeof(account.username), "halberdholder");
    redis_db->Hash.HSETALL(redis_db, 0, "1001_00000001", hdesc_tbls_account, (void *)&account);
    snprintf(account.username, sizeof(account.username), "halberdholder");
    snprintf(account.password, sizeof(account.password), "123445");
    redis_db->Hash.HMSET(redis_db, 0, "1001_00000001", hdesc_tbls_account, (void *)&account, "username", "password", NULL);
    redis_db->Hash.HSET2(redis_db, 0, "1001_00000001", "password", account.password);
    memset(&account, 0, sizeof(redis_account));
    redis_db->Hash.HGETALL(redis_db, 0, "1001_00000001", hdesc_tbls_account, (void *)&account);
    EMI_LOG("Hash.HGETALL\n");
    EMI_LOG("\tid: %d\n", account.id);
    EMI_LOG("\tusername: %s\n", account.username);
    EMI_LOG("\tpassword: %s\n", account.password);
    EMI_LOG("\tvip: %d\n", account.vip);
    redis_db->Key.DEL(redis_db, 0, "1001_00000001");
    EMI_LOG("TODO OTHER TEST\n");
    EMI_LOG("\n");


	return 0;
}

