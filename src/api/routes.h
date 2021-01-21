/* Pi-hole: A black hole for Internet advertisements
*  (c) 2017 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  API route prototypes
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */
#ifndef ROUTES_H
#define ROUTES_H

// struct mg_connection
#include "../civetweb/civetweb.h"
// type cJSON
#include "../cJSON/cJSON.h"
#include "../webserver/http-common.h"

// API router
int api_handler(struct mg_connection *conn, void *ignored);

// Statistic methods
int api_stats_summary(struct mg_connection *conn);
int api_stats_overTime_history(struct mg_connection *conn);
int api_stats_overTime_clients(struct mg_connection *conn);
int api_stats_query_types(struct mg_connection *conn);
int api_stats_upstreams(struct mg_connection *conn);
int api_stats_top_domains(bool blocked, struct mg_connection *conn);
int api_stats_top_clients(bool blocked, struct mg_connection *conn);
int api_stats_history(struct mg_connection *conn);
int api_stats_recentblocked(struct mg_connection *conn);

// Statistics methods (database)
int api_stats_database_overTime_history(struct mg_connection *conn);
int api_stats_database_top_items(bool blocked, bool domains, struct mg_connection *conn);
int api_stats_database_summary(struct mg_connection *conn);
int api_stats_database_overTime_clients(struct mg_connection *conn);
int api_stats_database_query_types(struct mg_connection *conn);
int api_stats_database_upstreams(struct mg_connection *conn);

// FTL methods
int api_ftl_client(struct mg_connection *conn);
int api_ftl_dnsmasq_log(struct mg_connection *conn);
int api_ftl_database(struct mg_connection *conn);
int api_ftl_system(struct mg_connection *conn);
int get_system_obj(struct mg_connection *conn, cJSON *system);

// Network methods
int api_network(struct mg_connection *conn);

// DNS methods
int api_dns_blockingstatus(struct mg_connection *conn);
int api_dns_cacheinfo(struct mg_connection *conn);

// List methods
int api_list(struct mg_connection *conn);
int api_group(struct mg_connection *conn);

// Version method
int api_version(struct mg_connection *conn);

// Auth method
int check_client_auth(struct mg_connection *conn, char payload[MAX_PAYLOAD_BYTES]);
int api_auth(struct mg_connection *conn);

// Settings methods
int api_settings_web(struct mg_connection *conn);

#endif // ROUTES_H
