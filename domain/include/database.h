#ifndef DATABASE_H
#define DATABASE_H

#include <sql.h>
#include <glib.h>

#define POOL_SIZE 50

typedef struct {
    SQLHENV hEnv;

    SQLHDBC hDbc[POOL_SIZE];
    int busy[POOL_SIZE];
    int current_index;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Database;

typedef void* (*RowMapperFunction)(SQLHSTMT);

void database_disconnect(Database *database);
void database_connect(Database *database, const char *data_source, const char *user, const char *password);
void database_run_non_query(Database *database, const char *query, GList *params);
GList* database_run_query(Database *database, const char *query, GList *params, RowMapperFunction row_mapper);

#endif
