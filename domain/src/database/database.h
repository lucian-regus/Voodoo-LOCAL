#pragma once

#include <libpq-fe.h>
#include <pthread.h>
#include "domain.h"
#include <glib.h>

#define MAX_CONNECTIONS 50

typedef struct {
    PGconn *connections[MAX_CONNECTIONS];
    int busy[MAX_CONNECTIONS];

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Database;

typedef GList* (*RowMapperFunction)(const PGresult*);

void database_cleanup(Database *database);
Database* database_init();
GList* run_query(Database* database, const char *query, RowMapperFunction row_mapper);