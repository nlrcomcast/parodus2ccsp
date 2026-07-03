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
 * invokes the target method synchronously, and serializes the method outParams
 * to a base64-encoded JSON string returned via @p result. Every allocation made
 * internally is freed before returning.
 *
 * @param encodedValue base64-encoded JSON payload from the RDK.Operate parameter.
 * @param result       out param receiving a newly allocated, base64-encoded JSON
 *                     result string on success (caller frees). Set to NULL on
 *                     failure.
 * @return WDMP_SUCCESS on success, a WDMP failure status otherwise.
 */
WDMP_STATUS webpaRbusOperate(const char *encodedValue, char **result);

#endif
