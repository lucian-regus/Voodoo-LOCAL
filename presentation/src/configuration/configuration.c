#include "configuration.h"

Database initialize_database() {
    Database database;

    const char *database_url = getenv("DATABASE_URL");
    const char *user = getenv("USER");
    const char *password = getenv("PASSWORD");

    database_connect(&database, database_url, user, password);

    return database;
}