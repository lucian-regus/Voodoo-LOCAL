#include <stdio.h>
#include <libpq-fe.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <pthread.h>

#include "domain.h"

PGconn* get_connection(Database *database) {
    PGconn *connection = NULL;
    pthread_mutex_lock(&database->mutex);

    while (1) {
        for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
            if (database->busy[i] == 0) {
                database->busy[i] = 1;
                connection = database->connections[i];
                pthread_mutex_unlock(&database->mutex);
                return connection;
            }
        }
        pthread_cond_wait(&database->cond, &database->mutex);
    }
}

void release_connection(Database *database, const PGconn *connection) {
    pthread_mutex_lock(&database->mutex);

    for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
        if (database->connections[i] == connection) {
            database->busy[i] = 0;
            break;
        }
    }

    pthread_cond_signal(&database->cond);
    pthread_mutex_unlock(&database->mutex);
}

void database_cleanup(Database *database) {
    if (database == NULL) {
        exit(1);
    }

    for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
        PQfinish(database->connections[i]);
    }

    pthread_mutex_destroy(&database->mutex);
    pthread_cond_destroy(&database->cond);

    free(database);
}

void exit_on_error(Database *database, const PGconn *connection) {
    fprintf(stderr, "Error: %s", PQerrorMessage(connection));

    database_cleanup(database);

    exit(1);
}

Database* database_init() {
    Database *database = malloc(sizeof(Database));

    pthread_mutex_init(&database->mutex, NULL);
    pthread_cond_init(&database->cond, NULL);


    const char *user = getenv("DATABASE_USER");
    const char *password = getenv("DATABASE_PASSWORD");

    char connection_info[512];
    snprintf(connection_info, sizeof(connection_info),"host=localhost port=6432 dbname=VOODOO-LOCAL user=%s password=%s", user, password);

    for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
        database->connections[i] = PQconnectdb(connection_info);
        database->busy[i] = 0;
        if (PQstatus(database->connections[i]) != CONNECTION_OK) {
            exit_on_error(database, database->connections[i]);
        }
    }

    return database;
}

GList* run_query(Database* database, const char *query, GList *params, RowMapperFunction row_mapper) {
    if (database == NULL) {
        exit(1);
    }

    PGconn *connection = get_connection(database);
    if (PQstatus(connection) != CONNECTION_OK) {
        exit_on_error(database, connection);
    }

    int param_count = g_list_length(params);
    const char **param_values = (param_count > 0)
        ? g_new0(const char*, param_count)
        : NULL;

    int i = 0;
    for (GList *iter = params; iter; iter = iter->next, i++) {
        param_values[i] = iter->data;
    }

    PGresult *response = PQexecParams(connection,
                                      query,
                                      param_count,
                                      NULL,
                                      param_values,
                                      NULL,
                                      NULL,
                                      0);
    g_free(param_values);
    if (PQresultStatus(response) != PGRES_TUPLES_OK) {
        PQclear(response);
        exit_on_error(database, connection);
    }

    release_connection(database, connection);

    GList* result = row_mapper(response);

    PQclear(response);

    return result;
}

void run_non_query(Database* database, const char *query, GList *params) {
    if (database == NULL) {
        exit(1);
    }

    PGconn *connection = get_connection(database);
    if (PQstatus(connection) != CONNECTION_OK) {
        exit_on_error(database, connection);
    }

    int param_count = g_list_length(params);
    const char **param_values = g_new(const char*, param_count);

    int i = 0;
    for (GList *iter = params; iter != NULL; iter = iter->next, i++) {
        param_values[i] = (const char*)iter->data;
    }

    PGresult *response = PQexecParams(connection,
                                      query,
                                      param_count,
                                      NULL,
                                      param_values,
                                      NULL,
                                      NULL,
                                      0);

    g_free(param_values);
    if (PQresultStatus(response) != PGRES_COMMAND_OK) {
        PQclear(response);
        exit_on_error(database, connection);
    }

    release_connection(database, connection);

    PQclear(response);
}