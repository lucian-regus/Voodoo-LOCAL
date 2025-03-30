#pragma once

#define MAX_RULE_LENGTH 2500
#define MAX_IP_LENGTH 40
#define MAX_SIGNATURE_LENGTH 513

typedef struct {
    int id;
    char rule[MAX_RULE_LENGTH];
} YaraRule;

typedef struct {
    int id;
    char ip_address[MAX_IP_LENGTH];
} BlacklistedIpAddress;

typedef struct {
    int id;
    char signature[MAX_SIGNATURE_LENGTH];
} MalwareSignature;