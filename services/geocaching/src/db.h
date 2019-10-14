#ifndef DB_H
#define DB_H

#include "geocacher.pb-c.h"

#define STORAGE_PASSWORD_LENGTH 8

Status put_flag(StoreSecretRequest *msg, unsigned char *password);
Status get_flag(GetSecretRequest *msg, unsigned char **output);
Status delete_flag(DiscardSecretRequest *msg);

int get_number_of_flags_to_list();
Status list_flags(ListAllBusyCellsResponse *msg);

int  db_connect();
void db_disconnect();
void dump_fast_storage();

#endif
