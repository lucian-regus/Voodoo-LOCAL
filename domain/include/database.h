#ifndef DATABASE_H
#define DATABASE_H

#include <sql.h>
#include <glib.h>

typedef struct {
    SQLHENV hEnv;
    SQLHDBC hDbc;
} Database;

void database_disconnect(Database *database);
void database_connect(Database *database, const char *dataSource, const char *user, const char *password);
void database_run_non_query(Database *database, const char *query, GList *params);

#endif
