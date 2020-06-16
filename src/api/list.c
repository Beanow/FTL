/* Pi-hole: A black hole for Internet advertisements
*  (c) 2020 Pi-hole, LLC (https://pi-hole.net)
*  Network-wide ad blocking via your own hardware.
*
*  FTL Engine
*  API Implementation /api/{white,black}list
*
*  This file is copyright under the latest version of the EUPL.
*  Please see LICENSE file for your rights under this license. */

#include "../FTL.h"
#include "../webserver/http-common.h"
#include "../webserver/json_macros.h"
#include "routes.h"
#include "../database/gravity-db.h"

static int getTableType(bool whitelist, bool exact)
{
	if(whitelist)
		if(exact)
			return GRAVITY_DOMAINLIST_EXACT_WHITELIST;
		else
			return GRAVITY_DOMAINLIST_REGEX_WHITELIST;
	else
		if(exact)
			return GRAVITY_DOMAINLIST_EXACT_BLACKLIST;
		else
			return GRAVITY_DOMAINLIST_REGEX_BLACKLIST;
}

static int api_dns_domainlist_read(struct mg_connection *conn, bool exact, bool whitelist)
{
	int type = getTableType(whitelist, exact);
	const char *sql_msg = NULL;
	if(!gravityDB_readTable(type, &sql_msg))
	{
		cJSON *json = JSON_NEW_OBJ();

		// Add SQL message (may be NULL = not available)
		if (sql_msg != NULL) {
			JSON_OBJ_REF_STR(json, "sql_msg", sql_msg);
		} else {
			JSON_OBJ_ADD_NULL(json, "sql_msg");
		}

		return send_json_error(conn, 402, // 402 Request Failed
		                       "database_error",
		                       "Could not read domains from database table",
		                       json);
	}

	domainrecord domain;
	cJSON *json = JSON_NEW_ARRAY();
	while(gravityDB_readTableGetDomain(&domain, &sql_msg))
	{
		cJSON *item = JSON_NEW_OBJ();
		JSON_OBJ_COPY_STR(item, "domain", domain.domain);
		JSON_OBJ_ADD_BOOL(item, "enabled", domain.enabled);
		JSON_OBJ_ADD_NUMBER(item, "date_added", domain.date_added);
		JSON_OBJ_ADD_NUMBER(item, "date_modified", domain.date_modified);
		JSON_OBJ_COPY_STR(item, "comment", domain.comment);
		JSON_ARRAY_ADD_ITEM(json, item);
	}
	gravityDB_readTableFinalize();

	if(sql_msg == NULL)
	{
		// No error
		JSON_SEND_OBJECT(json);
	}
	else
	{
		JSON_DELETE(json);
		json = JSON_NEW_OBJ();

		// Add SQL message (may be NULL = not available)
		if (sql_msg != NULL) {
			JSON_OBJ_REF_STR(json, "sql_msg", sql_msg);
		} else {
			JSON_OBJ_ADD_NULL(json, "sql_msg");
		}

		return send_json_error(conn, 402, // 402 Request Failed
		                       "database_error",
		                       "Could not read domains from database table",
		                       json);
	}
}

static int api_dns_domainlist_POST(struct mg_connection *conn,
                                   bool exact,
                                   bool whitelist)
{
	char buffer[1024];
	int data_len = mg_read(conn, buffer, sizeof(buffer) - 1);
	if ((data_len < 1) || (data_len >= (int)sizeof(buffer))) {
		return send_json_error(conn, 400,
                                       "bad_request", "No request body data",
                                       NULL);
	}
	buffer[data_len] = '\0';

	cJSON *obj = cJSON_Parse(buffer);
	if (obj == NULL) {
		return send_json_error(conn, 400,
		                       "bad_request",
		                       "Invalid request body data",
		                       NULL);
	}

	cJSON *elem = cJSON_GetObjectItemCaseSensitive(obj, "domain");
	if (!cJSON_IsString(elem)) {
		cJSON_Delete(obj);
		return send_json_error(conn, 400,
		                "bad_request",
		                "No \"domain\" string in body data",
		                NULL);
	}
	char *domain = strdup(elem->valuestring);

	bool enabled = true;
	elem = cJSON_GetObjectItemCaseSensitive(obj, "enabled");
	if (cJSON_IsBool(elem)) {
		enabled = elem->type == cJSON_True;
	}

	char *comment = NULL;
	elem = cJSON_GetObjectItemCaseSensitive(obj, "comment");
	if (cJSON_IsString(elem)) {
		comment = strdup(elem->valuestring);
	}
	cJSON_Delete(obj);

	cJSON *json = JSON_NEW_OBJ();
	int type = getTableType(whitelist, exact);
	const char *sql_msg = NULL;
	if(gravityDB_addToTable(type, domain, enabled, comment, &sql_msg))
	{
		// Add domain
		JSON_OBJ_COPY_STR(json, "domain", domain);
		free(domain);

		// Add enabled boolean
		JSON_OBJ_ADD_BOOL(json, "enabled", enabled);

		// Add comment (may be NULL)
		if (comment != NULL) {
			JSON_OBJ_COPY_STR(json, "comment", comment);
			free(comment);
		} else {
			JSON_OBJ_ADD_NULL(json, "comment");
		}

		// Send success reply
		JSON_SEND_OBJECT_CODE(json, 201); // 201 Created
	}
	else
	{
		// Add domain
		JSON_OBJ_COPY_STR(json, "domain", domain);
		free(domain);

		// Add enabled boolean
		JSON_OBJ_ADD_BOOL(json, "enabled", enabled);

		// Add comment (may be NULL)
		if (comment != NULL) {
			JSON_OBJ_COPY_STR(json, "comment", comment);
			free(comment);
		} else {
			JSON_OBJ_ADD_NULL(json, "comment");
		}

		// Add SQL message (may be NULL = not available)
		if (sql_msg != NULL) {
			JSON_OBJ_REF_STR(json, "sql_msg", sql_msg);
		} else {
			JSON_OBJ_ADD_NULL(json, "sql_msg");
		}

		// Send error reply
		return send_json_error(conn, 402, // 402 Request Failed
		                       "database_error",
		                       "Could not add domain to gravity database",
		                       json);
	}
}

static int api_dns_domainlist_DELETE(struct mg_connection *conn,
                                   bool exact,
                                   bool whitelist)
{
	const struct mg_request_info *request = mg_get_request_info(conn);

	char domain[1024];
	// Advance one character to strip "/"
	const char *encoded_uri = strrchr(request->local_uri, '/')+1u;
	// Decode URL (necessary for regular expressions, harmless for domains)
	mg_url_decode(encoded_uri, strlen(encoded_uri), domain, sizeof(domain)-1u, 0);

	cJSON *json = JSON_NEW_OBJ();
	int type = getTableType(whitelist, exact);
	const char *sql_msg = NULL;
	if(gravityDB_delFromTable(type, domain, &sql_msg))
	{
		JSON_OBJ_REF_STR(json, "key", "removed");
		JSON_OBJ_REF_STR(json, "domain", domain);
		JSON_SEND_OBJECT_CODE(json, 200); // 200 OK
	}
	else
	{
		// Add domain
		JSON_OBJ_REF_STR(json, "domain", domain);

		// Add SQL message (may be NULL = not available)
		if (sql_msg != NULL) {
			JSON_OBJ_REF_STR(json, "sql_msg", sql_msg);
		} else {
			JSON_OBJ_ADD_NULL(json, "sql_msg");
		}

		// Send error reply
		return send_json_error(conn, 402,
		                       "database_error",
		                       "Could not remove domain from database table",
		                       json);
	}
}

int api_dns_domainlist(struct mg_connection *conn, bool exact, bool whitelist)
{
	// Verify requesting client is allowed to see this ressource
	if(check_client_auth(conn) < 0)
	{
		return send_json_unauthorized(conn);
	}

	int method = http_method(conn);
	if(method == HTTP_GET)
	{
		return api_dns_domainlist_read(conn, exact, whitelist);
	}
	else if(method == HTTP_PUT)
	{
		// Add domain from exact white-/blacklist when a user sends
		// the request to the general address /api/dns/{white,black}list
		return api_dns_domainlist_POST(conn, exact, whitelist);
	}
	else if(method == HTTP_DELETE)
	{
		// Delete domain from exact white-/blacklist when a user sends
		// the request to the general address /api/dns/{white,black}list
		return api_dns_domainlist_DELETE(conn, exact, whitelist);
	}
	else
	{
		// This results in error 404
		return 0;
	}
}
