#pragma once

GList* map_yara_rule(const PGresult *response);
GList* map_malware_signature(const PGresult *response);
GList* map_blacklisted_ip_addresses(const PGresult *response);