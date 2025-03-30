#include <stdio.h>

#include "domain.h"

int main() {
    Database* database = database_init();


    database_cleanup(database);
}