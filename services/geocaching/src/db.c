#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <stdio.h>

#include "db.h"
#include "debug.h"
#include "custom_malloc.h"

#define FAST_STORAGE_SIZE 10

const int SQLITE_TIMEOUT = 4000;

typedef struct __attribute__((__packed__))
{
    unsigned int lat;
    unsigned int lon;
    unsigned char pwd[STORAGE_PASSWORD_LENGTH];
    unsigned char data[];
} FastStorage;

FastStorage *fast_storage[FAST_STORAGE_SIZE];
sqlite3 *db;
int db_connected = 0;

Status store_item(unsigned int lat, unsigned int lon, unsigned char *password,
                  unsigned char *data, size_t data_len)
{
    sqlite3_stmt *pStmt;
    int rc;

    char *sql = "INSERT INTO secrets VALUES(?,?,?,?)";
    rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);

    if (rc != SQLITE_OK) {
        LOG("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        return STATUS__FAIL;
    }

    sqlite3_bind_int(pStmt, 1, lat);
    sqlite3_bind_int(pStmt, 2, lon);
    sqlite3_bind_blob(pStmt, 3, password, STORAGE_PASSWORD_LENGTH, SQLITE_STATIC);
    sqlite3_bind_blob(pStmt, 4, data, data_len, SQLITE_STATIC);

    rc = sqlite3_step(pStmt);

    if (rc != SQLITE_DONE) {
        LOG("execution failed: %s", sqlite3_errmsg(db));
        sqlite3_finalize(pStmt);
        return STATUS__FAIL;
    }

    sqlite3_finalize(pStmt);
    return STATUS__OK;
}

Status put_flag(StoreSecretRequest *msg, unsigned char *password)
{
    for (int i = 0; i < FAST_STORAGE_SIZE; i++)
    {
        if (fast_storage[i])
        {
            continue;
        }
        unsigned int alloc_size = 0;
        if (msg->has_size_hint)
        {
            alloc_size = msg->size_hint;
        }
        if (alloc_size < msg->secret.len)
        {
            alloc_size = msg->secret.len;
        }

        if (msg->has_key)
        {
            memcpy(password, msg->key.data, STORAGE_PASSWORD_LENGTH);
        }

        // BUG: Allocated memory is 1 byte shorter than it should be
        fast_storage[i] = malloc(alloc_size + sizeof(FastStorage) - 1);
        fast_storage[i]->lat = msg->coordinates->lat;
        fast_storage[i]->lon = msg->coordinates->lon;
        memcpy(fast_storage[i]->pwd, password, STORAGE_PASSWORD_LENGTH);
        memcpy(fast_storage[i]->data, msg->secret.data, msg->secret.len);

        return STATUS__OK;
    }
    LOG("No space in fast storage, storing to db\n");

    if (db_connect()) {
        return STATUS__FAIL;
    }
    return store_item(msg->coordinates->lat, msg->coordinates->lon, password,
                      msg->secret.data, msg->secret.len);
}

void dump_fast_storage() {
    for (int i = 0; i < FAST_STORAGE_SIZE; i++)
    {
        if (!fast_storage[i])
        {
            continue;
        }
        store_item(fast_storage[i]->lat, fast_storage[i]->lon, fast_storage[i]->pwd,
                   fast_storage[i]->data, strlen(fast_storage[i]->data));
    }
}

Status get_flag(GetSecretRequest *msg, unsigned char **output)
{
    for (int i = 0; i < FAST_STORAGE_SIZE; i++)
    {
        if (!fast_storage[i])
        {
            continue;
        }
        if (fast_storage[i]->lat != msg->coordinates->lat)
        {
            continue;
        }
        if (fast_storage[i]->lon != msg->coordinates->lon)
        {
            continue;
        }
        if (memcmp(fast_storage[i]->pwd, msg->key.data, STORAGE_PASSWORD_LENGTH))
        {
            continue;
        }
        *output = &fast_storage[i]->data[0];
        return STATUS__OK;
    }
    LOG("Not found in fast storage, looking in db\n");

    if (db_connect()) {
        return STATUS__FAIL;
    }

    sqlite3_stmt *pStmt;
    int rc;

    char *sql = "SELECT data FROM secrets where lat = ? and lon = ? and key = ?";
    rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);

    if (rc != SQLITE_OK) {
        LOG("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        return STATUS__FAIL;
    }

    sqlite3_bind_int(pStmt, 1, msg->coordinates->lat);
    sqlite3_bind_int(pStmt, 2, msg->coordinates->lon);
    sqlite3_bind_blob(pStmt, 3, msg->key.data, STORAGE_PASSWORD_LENGTH, SQLITE_STATIC);

    rc = sqlite3_step(pStmt);

    if (rc == SQLITE_ROW) {
        const unsigned char *data = sqlite3_column_blob(pStmt, 0);
        int data_size = sqlite3_column_bytes(pStmt, 0);

        *output = my_malloc(data_size + 1);
        memcpy(*output, data, data_size);
        (*output)[data_size] = 0;
        sqlite3_finalize(pStmt);
        return STATUS__OK;

    } else if (rc == SQLITE_DONE) {
        sqlite3_finalize(pStmt);
        return STATUS__NOT_FOUND;
    }

    LOG("execution failed: %s", sqlite3_errmsg(db));
    sqlite3_finalize(pStmt);
    return STATUS__FAIL;
}

Status delete_flag(DiscardSecretRequest *msg)
{
    for (int i = 0; i < FAST_STORAGE_SIZE; i++)
    {
        if (!fast_storage[i])
        {
            continue;
        }
        if (fast_storage[i]->lat != msg->coordinates->lat)
        {
            continue;
        }
        if (fast_storage[i]->lon != msg->coordinates->lon)
        {
            continue;
        }
        if (memcmp(fast_storage[i]->pwd, msg->key.data, STORAGE_PASSWORD_LENGTH))
        {
            continue;
        }
        free(fast_storage[i]);
        fast_storage[i] = 0;
        return STATUS__OK;
    }

    LOG("Couldn't find secret in fast storage, looking in DB\n");
    if (db_connect()) {
        return STATUS__FAIL;
    }

    sqlite3_stmt *pStmt;
    int rc;

    char *sql = "DELETE FROM secrets where lat = ? and lon = ? and key = ?";
    rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);

    if (rc != SQLITE_OK) {
        LOG("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        return STATUS__FAIL;
    }

    sqlite3_bind_int(pStmt, 1, msg->coordinates->lat);
    sqlite3_bind_int(pStmt, 2, msg->coordinates->lon);
    sqlite3_bind_blob(pStmt, 3, msg->key.data, STORAGE_PASSWORD_LENGTH, SQLITE_STATIC);

    rc = sqlite3_step(pStmt);

    if (rc == SQLITE_DONE) {
        sqlite3_finalize(pStmt);
        return STATUS__OK;
    }

    LOG("execution failed: %s", sqlite3_errmsg(db));
    sqlite3_finalize(pStmt);
    return STATUS__FAIL;
}

int  db_connect()
{
    if (db_connected)
    {
        return 0;
    }
#ifdef DEBUG
    int rc = sqlite3_open("secrets.db", &db);
#else
    int rc = sqlite3_open("data/secrets.db", &db);
#endif

    sqlite3_busy_timeout(db, SQLITE_TIMEOUT);
    if ( rc )
    {
        LOG("Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    char *sql = "CREATE TABLE IF NOT EXISTS secrets(lat INT, lon INT, key BLOB, data BLOB);";

    char *err_msg = 0;
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK )
    {
        LOG("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    db_connected = 1;
    return 0;
}

void db_disconnect()
{
    sqlite3_close(db);
    db_connected = 0;
}


int get_number_of_flags_to_list()
{
    int result = 0;

    if (db_connect()) {
        return result;
    }

    sqlite3_stmt *pStmt;
    int rc;

    char *sql = "SELECT count(*) FROM secrets where lat > 0 and lon > 0";
    rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);

    if (rc != SQLITE_OK) {
        LOG("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        return result;
    }

    rc = sqlite3_step(pStmt);

    if (rc == SQLITE_ROW) {
        result = sqlite3_column_int(pStmt, 0);
    }
    sqlite3_finalize(pStmt);
    return result;
}

Status list_flags(ListAllBusyCellsResponse *msg)
{
    if (db_connect()) {
        return STATUS__FAIL;
    }

    sqlite3_stmt *pStmt;
    int rc;

    char *sql = "SELECT data, lat, lon FROM secrets where lat > 0 and lon > 0";
    rc = sqlite3_prepare(db, sql, -1, &pStmt, 0);

    if (rc != SQLITE_OK) {
        LOG("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        msg->n_cells = 0;
        return STATUS__FAIL;
    }

    for (int i = 0; i < msg->n_cells; i++)
    {
        rc = sqlite3_step(pStmt);

        if (rc == SQLITE_ROW) {
            const unsigned char *data = sqlite3_column_blob(pStmt, 0);
            int data_size = sqlite3_column_bytes(pStmt, 0);

            msg->cells[i]->secret.data = my_malloc(data_size + 1);
            msg->cells[i]->secret.len = data_size;
            memcpy(msg->cells[i]->secret.data, data, data_size);
            msg->cells[i]->secret.data[data_size] = 0;

            msg->cells[i]->coordinates->lat = sqlite3_column_int(pStmt, 1);
            msg->cells[i]->coordinates->lon = sqlite3_column_int(pStmt, 2);
        } else if (rc == SQLITE_DONE) {
            msg->n_cells = i;
            break;
        } else {
            LOG("execution failed: %s", sqlite3_errmsg(db));
            sqlite3_finalize(pStmt);
            msg->n_cells = i;
            return STATUS__FAIL;
        }
    }

    sqlite3_finalize(pStmt);
    return STATUS__OK;
}
