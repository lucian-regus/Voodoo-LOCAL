#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "database.h"

#include <unistd.h>

void print_sql_error(SQLHSTMT hStmt) {
    SQLCHAR sqlState[6];
    SQLCHAR message[256];
    SQLINTEGER nativeError;
    SQLSMALLINT messageLength;

    SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);

    printf("SQL error: %s, %s\n", sqlState, message);
}

SQLHDBC get_connection(Database *database) {
    SQLHDBC connection = NULL;
    pthread_mutex_lock(&database->mutex);

    while (1) {
        for (int i = 0; i < 50; i++) {
            if (database->busy[i] == 0) {
                database->busy[i] = 1;
                connection = database->hDbc[i];
                pthread_mutex_unlock(&database->mutex);
                return connection;
            }
        }
        pthread_cond_wait(&database->cond, &database->mutex);
    }
}

void release_connection(Database *database, SQLHDBC connection) {
    pthread_mutex_lock(&database->mutex);
    for (int i = 0; i < POOL_SIZE; i++) {
        if (database->hDbc[i] == connection) {
            database->busy[i] = 0;
            break;
        }
    }

    pthread_cond_signal(&database->cond);
    pthread_mutex_unlock(&database->mutex);
}

void database_disconnect(Database *database) {
    for (int i = 0; i < POOL_SIZE; i++) {
        SQLDisconnect(database->hDbc[i]);
        SQLFreeHandle(SQL_HANDLE_DBC, database->hDbc[i]);
    }
    if (database->hEnv != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, database->hEnv);
    }

    pthread_mutex_destroy(&database->mutex);
    pthread_cond_destroy(&database->cond);
}

void database_connect(Database *database, const char *dataSource, const char *user, const char *password) {
    SQLRETURN ret;

    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &database->hEnv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error allocating environment handle\n");
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    ret = SQLSetEnvAttr(database->hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error setting ODBC version\n");
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    for (int i = 0 ; i < POOL_SIZE ; i++) {
        ret = SQLAllocHandle(SQL_HANDLE_DBC, database->hEnv, &database->hDbc[i]);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            printf("Error allocating connection handle\n");
            database_disconnect(database);
            exit(EXIT_FAILURE);

        }

        ret = SQLConnect(database->hDbc[i],
        (SQLCHAR *)dataSource,
        SQL_NTS,
        (SQLCHAR *)user,
        SQL_NTS,
        (SQLCHAR *)password,
        SQL_NTS);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            printf("Error connecting to the database\n");
            database_disconnect(database);
            exit(EXIT_FAILURE);
        }

        database->busy[i] = 0;
    }

    pthread_mutex_init(&database->mutex, NULL);
    pthread_cond_init(&database->cond, NULL);
}

void database_run_non_query(Database *database, const char *query, GList *params) {
    SQLRETURN ret;
    SQLHSTMT hStmt;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, database->hDbc, &hStmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error allocation SQL Statement Handle\n");
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    ret = SQLPrepare(hStmt, (SQLCHAR *)query, SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error preparing SQL query\n");
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    GList *current = params;
    int index = 0;
    while (current != NULL) {
        ret = SQLBindParameter(hStmt, index + 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, (SQLPOINTER)current->data, 0, NULL);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            printf("Error binding parameter %d\n", index + 1);
            database_disconnect(database);
            exit(EXIT_FAILURE);
        }
        current = current->next;
        index++;
    }

    ret = SQLExecute(hStmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error executing query\n");
        print_sql_error(hStmt);
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

GList* database_run_query(Database *database, const char *query, GList *params, RowMapperFunction row_mapper) {
    SQLRETURN ret;
    SQLHSTMT hStmt;

    SQLHDBC connection = get_connection(database);

    ret = SQLAllocHandle(SQL_HANDLE_STMT, connection, &hStmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error allocation SQL Statement Handle\n");
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    ret = SQLPrepare(hStmt, (SQLCHAR *)query, SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error preparing SQL query\n");
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    GList *current = params;
    int index = 0;
    while (current != NULL) {
        ret = SQLBindParameter(hStmt, index + 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0, (SQLPOINTER)current->data, 0, NULL);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            printf("Error binding parameter %d\n", index + 1);
            database_disconnect(database);
            exit(EXIT_FAILURE);
        }
        current = current->next;
        index++;
    }

    ret = SQLExecute(hStmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error executing query\n");
        print_sql_error(hStmt);
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    GList *result = NULL;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        result = g_list_append(result, row_mapper(hStmt));
    }

    release_connection(database, connection);

    return result;
}