#ifndef _WEBPA_RBUS_H_
#define _WEBPA_RBUS_H_

#include <stdio.h>
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>

#include "webpa_adapter.h"
#include <wdmp-c.h>
#include <cimplog.h>
#include <cJSON.h>


bool isRbusEnabled();
bool isRbusInitialized();
WDMP_STATUS webpaRbusInit(const char *pComponentName);
void webpaRbus_Uninit();
rbusError_t setTraceContext(char* traceContext[]);
rbusError_t getTraceContext(char* traceContext[]);
rbusError_t clearTraceContext();

/* WebPA OPERATE (method invocation) support.
 * The reserved SET parameter name that routes a request to method invocation
 * instead of a standard parameter write. */
#define WEBPA_OPERATE_PARAM_NAME "RDK.Operate"

/* Maximum accepted size (in bytes) of the base64-decoded RDK.Operate payload. */
#define WEBPA_OPERATE_MAX_PAYLOAD_SIZE (64 * 1024)

/* JSON-RPC error codes returned inside the OPERATE response "message" (base64
 * of {"error":{"code":<code>,"data":<data>}}). Taken from the JSON-RPC spec. */
#define OPERATE_JSONRPC_PARSE_ERROR      (-32700) /* base64 decode or JSON parse failed */
#define OPERATE_JSONRPC_INVALID_REQUEST  (-32600) /* request not valid / properly formatted */
#define OPERATE_JSONRPC_METHOD_NOT_FOUND (-32601) /* no rbus provider provides this method */
#define OPERATE_JSONRPC_INVALID_PARAMS   (-32602) /* provider returned RBUS_ERROR_INVALID_INPUT */
#define OPERATE_JSONRPC_INTERNAL_ERROR   (-32603) /* catch-all internal failure */

/**
 * @brief Build a flat rbusObject_t from a decoded params.parameters cJSON object.
 *
 * Each entry key becomes a property whose value is set from the corresponding
 * "value" field, mapped from the per-entry WDMP "dataType" to an rbus value type
 * (defaulting to string). The resulting object is flat (no nested wrapper) so
 * that provider handlers reading rbusObject_GetValue(inParams, "<key>") work
 * unchanged. The caller owns the returned object and must release it with
 * rbusObject_Release.
 *
 * @param parameters cJSON object of key -> { value, dataType }. May be NULL/empty.
 * @param inParams   out param receiving the newly created rbusObject_t.
 * @return WDMP_SUCCESS on success, a WDMP failure status otherwise.
 */
WDMP_STATUS buildOperateInParams(cJSON *parameters, rbusObject_t *inParams);

/**
 * @brief Synchronously invoke an RBUS method.
 *
 * Wraps rbusMethod_Invoke using the internal rbus handle.
 *
 * @param methodName fully-qualified RBUS method name.
 * @param inParams   flat input params (may be NULL for no params).
 * @param outParams  out param receiving the method result object (may be NULL).
 * @return the rbusError_t result code from rbusMethod_Invoke.
 */
rbusError_t webpaRbusMethodInvoke(const char *methodName, rbusObject_t inParams, rbusObject_t *outParams);

/**
 * @brief Handle a WebPA OPERATE request end to end.
 *
 * Base64-decodes the RDK.Operate value, parses the JSON, builds flat inParams,
 * invokes the target method synchronously, and produces a base64-encoded JSON
 * "message" for the response. On success the message wraps the method outParams
 * as {"result": {...}}; on failure it wraps a JSON-RPC error as
 * {"error": {"code": <code>, "data": <data>}}. The invoked method name is also
 * returned so the response "name" field can echo it. Every internal allocation
 * is freed before returning.
 *
 * @param encodedValue base64-encoded JSON payload from the RDK.Operate parameter.
 * @param methodName   out param receiving a newly allocated copy of the invoked
 *                     method name (caller frees), or NULL when it could not be
 *                     determined (e.g. decode/parse failure).
 * @param result       out param receiving a newly allocated, base64-encoded JSON
 *                     message (caller frees) for both success and failure.
 * @return WDMP_SUCCESS on success, a WDMP failure status otherwise.
 */
WDMP_STATUS webpaRbusOperate(const char *encodedValue, char **methodName, char **result);

#endif
