#include <libpq-fe.h>
#include <string.h>
#include <glib.h>

#include "domain.h"

GList* map_yara_rule(const PGresult *response) {
    GList *result = NULL;

    const int rows = PQntuples(response);

    for (int row = 0; row < rows; row++) {
        YaraRule *yara_rule = g_malloc(sizeof(YaraRule));
        yara_rule->id = atoi(PQgetvalue(response, row, 0));

        strncpy(yara_rule->rule, PQgetvalue(response, row, 1), MAX_RULE_LENGTH - 1);
        yara_rule->rule[MAX_RULE_LENGTH - 1] = '\0';

        result = g_list_append(result, yara_rule);
    }

    return result;
}

GList* map_malware_signature(const PGresult *response) {
    GList *result = NULL;

    const int rows = PQntuples(response);

    for (int row = 0; row < rows; row++) {
        MalwareSignature *malware_signature = malloc(sizeof(MalwareSignature));
        malware_signature->id = atoi(PQgetvalue(response, row, 0));

        strncpy(malware_signature->signature, PQgetvalue(response, row, 1), MAX_SIGNATURE_LENGTH - 1);
        malware_signature->signature[MAX_SIGNATURE_LENGTH - 1] = '\0';

        result = g_list_append(result, malware_signature);
    }

    return result;
}

GList* map_blacklisted_ip_addresses(const PGresult *response) {
    GList *result = NULL;

    const int rows = PQntuples(response);

    for (int row = 0; row < rows; row++) {
        BlacklistedIpAddress *blacklisted_ip_addresses = malloc(sizeof(BlacklistedIpAddress));;
        blacklisted_ip_addresses->id = atoi(PQgetvalue(response, row, 0));

        strncpy(blacklisted_ip_addresses->ip_address, PQgetvalue(response, row, 1), MAX_IP_LENGTH - 1);
        blacklisted_ip_addresses->ip_address[MAX_IP_LENGTH - 1] = '\0';

        result = g_list_append(result, blacklisted_ip_addresses);
    }

    return result;
}