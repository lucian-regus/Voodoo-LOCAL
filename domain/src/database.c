#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

typedef struct {
    SQLHENV hEnv;
    SQLHDBC hDbc;
} Database;

void print_sql_error(SQLHSTMT hStmt) {
    SQLCHAR sqlState[6];
    SQLCHAR message[256];
    SQLINTEGER nativeError;
    SQLSMALLINT messageLength;

    SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);

    printf("SQL error: %s, %s\n", sqlState, message);
}

void database_disconnect(Database *database) {
    if (database->hDbc) {
        SQLDisconnect(database->hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, database->hDbc);
    }

    if (database->hEnv) {
        SQLFreeHandle(SQL_HANDLE_ENV, database->hEnv);
    }

    database->hDbc=NULL;
    database->hEnv = NULL;
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

    ret = SQLAllocHandle(SQL_HANDLE_DBC, database->hEnv, &database->hDbc);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("Error allocating connection handle\n");
        database_disconnect(database);
        exit(EXIT_FAILURE);
    }

    ret = SQLConnect(database->hDbc,
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