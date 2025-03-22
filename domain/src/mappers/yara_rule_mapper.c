#include <sql.h>
#include <sqlext.h>
#include <stdlib.h>
#include <glib.h>

#include "models.h"

void *map_yara_rule(SQLHSTMT hstmt) {
    YaraRule *rule = g_new(YaraRule, 1);

    SQLGetData(hstmt, 1, SQL_C_CHAR, rule->id, sizeof(rule->id), NULL);
    SQLGetData(hstmt, 2, SQL_C_CHAR, rule->rule, sizeof(rule->rule), NULL);

    return rule;
}