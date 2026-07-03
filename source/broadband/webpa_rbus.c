#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <pthread.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include <cJSON.h>
#include <trower-base64/base64.h>
#include "webpa_rbus.h"

static rbusHandle_t rbus_handle;
static bool isRbus = false;

/* Device MAC (defined in webpa_notification.c); used to build the
 * "mac:<deviceMAC>" source for asynchronous OPERATE result events.
 * getDeviceMac() populates the global on first use. */
extern char deviceMAC[32];
extern void getDeviceMac();

bool isRbusEnabled()
{
        if(RBUS_ENABLED == rbus_checkStatus())
        {
                isRbus = true;
        }
        else
        {
                isRbus = false;
        }
        WalInfo("Webpa RBUS mode active status = %s\n", isRbus ? "true":"false");
        return isRbus;
}

bool isRbusInitialized()
{
    return rbus_handle != NULL ? true : false;
}

WDMP_STATUS webpaRbusInit(const char *pComponentName)
{
        int ret = RBUS_ERROR_SUCCESS;

        WalInfo("rbus_open for component %s\n", pComponentName);
        ret = rbus_open(&rbus_handle, pComponentName);
        if(ret != RBUS_ERROR_SUCCESS)
        {
                WalError("webpaRbusInit failed with error code %d\n", ret);
                return WDMP_FAILURE;
        }
        WalInfo("webpaRbusInit is success. ret is %d\n", ret);
        return WDMP_SUCCESS;
}

void webpaRbus_Uninit()
{
    rbus_close(rbus_handle);
}

rbusError_t setTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        /* CID-334852 Function address comparison fix */
        if(isRbusInitialized())
        {
                if(traceContext[0] != NULL && traceContext[1] != NULL) {
                       if(strlen(traceContext[0]) > 0 && strlen(traceContext[1]) > 0) {
			    WalInfo("Invoked setTraceContext function with value traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);    
                            ret = rbusHandle_SetTraceContextFromString(rbus_handle, traceContext[0], traceContext[1]);
                            if(ret == RBUS_ERROR_SUCCESS) {
                                  WalPrint("SetTraceContext request success\n");
                            }
                             else {
                                   WalError("SetTraceContext request failed with error code - %d\n", ret);
                             }
                        }
                        else {
                              WalError("Header is empty\n");
                        }
                  }
                  else {
                        WalError("Header is NULL\n");
                  }
        }
        else {
                WalError("Rbus not initialzed in setTraceContext function\n");
        }	
        return ret;
}

rbusError_t getTraceContext(char* traceContext[])
{
        rbusError_t ret = RBUS_ERROR_BUS_ERROR;
        char traceParent[512] = {'\0'};
        char traceState[512] = {'\0'};
	/* CID-334847 Function address comparison fix */
	if(isRbusInitialized())
        {
	      ret =  rbusHandle_GetTraceContextAsString(rbus_handle, traceParent, sizeof(traceParent), traceState, sizeof(traceState));
	      if( ret == RBUS_ERROR_SUCCESS) {
		      if(strlen(traceParent) > 0 && strlen(traceState) > 0) {
			      WalPrint("GetTraceContext request success\n");
		              traceContext[0] = strdup(traceParent);
	                      traceContext[1] = strdup(traceState);
			      WalInfo("traceContext value, traceParent - %s, traceState - %s\n", traceContext[0], traceContext[1]);
	               }
		       else {
			       WalPrint("traceParent & traceState are empty\n");
		       }	       
	      }
	      else {
		      WalError("GetTraceContext request failed with error code - %d\n", ret);
	      }	      
	}
        else { 
              WalError("Rbus not initialzed in getTraceContext function\n");
	}
        return ret;
}

rbusError_t clearTraceContext()
{
	rbusError_t ret = RBUS_ERROR_BUS_ERROR;
	/* CID-334849 Function address comparison fix */
	if(isRbusInitialized())
	{
		ret = rbusHandle_ClearTraceContext(rbus_handle);
		if(ret == RBUS_ERROR_SUCCESS) {
			WalInfo("ClearTraceContext request success\n");
		}
		else {
			WalError("ClearTraceContext request failed with error code - %d\n", ret);
		}
	}
	else {
		WalError("Rbus not initialized in clearTraceContext funcion\n");
        }
        /* CID-334846 missing return statement fix */
        return ret;
}

/*----------------------------------------------------------------------------*/
/*                        WebPA OPERATE (method invoke)                       */
/*----------------------------------------------------------------------------*/

/* Map a per-entry WDMP dataType to an rbus value and set it on the object.
 * Defaults to a string value so providers calling rbusValue_GetString continue
 * to work regardless of the source data type. */
static void setRbusValueFromWdmp(rbusObject_t obj, const char *key, int dataType, const char *value)
{
        rbusValue_t rbusValue = NULL;
        rbusValue_Init(&rbusValue);

        switch(dataType)
        {
                case WDMP_INT:
                case WDMP_LONG:
                        rbusValue_SetInt32(rbusValue, (int32_t) strtol(value, NULL, 10));
                        break;
                case WDMP_BOOLEAN:
                        rbusValue_SetBoolean(rbusValue, (strcasecmp(value, "true") == 0));
                        break;
                default:
                        /* WDMP_STRING and everything else -> string */
                        rbusValue_SetString(rbusValue, value);
                        break;
        }

        rbusObject_SetValue(obj, key, rbusValue);
        rbusValue_Release(rbusValue);
}

WDMP_STATUS buildOperateInParams(cJSON *parameters, rbusObject_t *inParams)
{
        rbusObject_t obj = NULL;
        cJSON *entry = NULL;

        if(inParams == NULL)
        {
                return WDMP_ERR_INVALID_INPUT_PARAMETER;
        }

        rbusObject_Init(&obj, NULL);
        if(obj == NULL)
        {
                WalError("Failed to init rbusObject for OPERATE inParams\n");
                return WDMP_FAILURE;
        }

        /* An absent or empty "parameters" object yields an empty inParams. */
        if(parameters != NULL)
        {
                cJSON_ArrayForEach(entry, parameters)
                {
                        const char *key = entry->string;
                        cJSON *valueItem = NULL;
                        cJSON *typeItem = NULL;
                        int dataType = WDMP_STRING;

                        if(key == NULL)
                        {
                                continue;
                        }

                        valueItem = cJSON_GetObjectItem(entry, "value");
                        typeItem = cJSON_GetObjectItem(entry, "dataType");
                        if(typeItem != NULL && cJSON_IsNumber(typeItem))
                        {
                                dataType = typeItem->valueint;
                        }

                        if(valueItem != NULL && cJSON_IsString(valueItem) && valueItem->valuestring != NULL)
                        {
                                setRbusValueFromWdmp(obj, key, dataType, valueItem->valuestring);
                        }
                        else
                        {
                                /* No usable value -> set empty string so the key is still present. */
                                setRbusValueFromWdmp(obj, key, WDMP_STRING, "");
                        }
                }
        }

        *inParams = obj;
        return WDMP_SUCCESS;
}

rbusError_t webpaRbusMethodInvoke(const char *methodName, rbusObject_t inParams, rbusObject_t *outParams)
{
        if(!isRbusInitialized())
        {
                WalError("Rbus not initialized in webpaRbusMethodInvoke\n");
                return RBUS_ERROR_NOT_INITIALIZED;
        }
        if(methodName == NULL)
        {
                WalError("methodName is NULL in webpaRbusMethodInvoke\n");
                return RBUS_ERROR_INVALID_INPUT;
        }

        WalInfo("Invoking RBUS method %s\n", methodName);
        return rbusMethod_Invoke(rbus_handle, methodName, inParams, outParams);
}

/* Serialize a flat rbusObject_t of values into a cJSON object of strings. */
static cJSON *operateOutParamsToJson(rbusObject_t outParams)
{
        cJSON *json = cJSON_CreateObject();
        rbusProperty_t prop = NULL;

        if(json == NULL)
        {
                return NULL;
        }
        if(outParams == NULL)
        {
                return json;
        }

        prop = rbusObject_GetProperties(outParams);
        while(prop != NULL)
        {
                const char *name = rbusProperty_GetName(prop);
                rbusValue_t value = rbusProperty_GetValue(prop);
                if(name != NULL && value != NULL)
                {
                        char *str = rbusValue_ToString(value, NULL, 0);
                        cJSON_AddStringToObject(json, name, (str != NULL) ? str : "");
                        if(str != NULL)
                        {
                                free(str);
                        }
                }
                prop = rbusProperty_GetNext(prop);
        }
        return json;
}

/* Base64-encode a cJSON object's unformatted text. Returns a newly allocated
 * base64 string (caller frees) or NULL on failure. */
static char *operateJsonToBase64(cJSON *json)
{
        char *encoded = NULL;
        char *jsonText = NULL;

        if(json == NULL)
        {
                return NULL;
        }
        jsonText = cJSON_PrintUnformatted(json);
        if(jsonText != NULL)
        {
                size_t rawLen = strlen(jsonText);
                size_t encSize = b64_get_encoded_buffer_size(rawLen);
                encoded = (char *) malloc(encSize + 1);
                if(encoded != NULL)
                {
                        b64_encode((const uint8_t *) jsonText, rawLen, (uint8_t *) encoded);
                        encoded[encSize] = '\0';
                }
                free(jsonText);
        }
        return encoded;
}

/* Build a base64-encoded {"result": <outParams>} success message. */
static char *operateBuildSuccessMessage(rbusObject_t outParams)
{
        cJSON *root = cJSON_CreateObject();
        cJSON *resultObj = NULL;
        char *encoded = NULL;

        if(root == NULL)
        {
                return NULL;
        }
        resultObj = operateOutParamsToJson(outParams);
        if(resultObj == NULL)
        {
                resultObj = cJSON_CreateObject();
        }
        cJSON_AddItemToObject(root, "result", resultObj);
        encoded = operateJsonToBase64(root);
        cJSON_Delete(root);
        return encoded;
}

/* Build a base64-encoded {"error":{"code":<code>,"data":<data>}} failure
 * message. When data is NULL the "data" field is omitted. */
static char *operateBuildErrorMessage(int code, const char *data)
{
        cJSON *root = cJSON_CreateObject();
        cJSON *err = NULL;
        char *encoded = NULL;

        if(root == NULL)
        {
                return NULL;
        }
        err = cJSON_CreateObject();
        if(err == NULL)
        {
                cJSON_Delete(root);
                return NULL;
        }
        cJSON_AddItemToObject(root, "error", err);
        cJSON_AddNumberToObject(err, "code", code);
        if(data != NULL)
        {
                cJSON_AddStringToObject(err, "data", data);
        }
        encoded = operateJsonToBase64(root);
        cJSON_Delete(root);
        return encoded;
}

/* Map an rbus method invocation result to a WDMP status. */
static WDMP_STATUS mapRbusToWdmpStatus(rbusError_t rc)
{
        switch(rc)
        {
                case RBUS_ERROR_SUCCESS:
                        return WDMP_SUCCESS;
                case RBUS_ERROR_INVALID_INPUT:
                        return WDMP_ERR_INVALID_INPUT_PARAMETER;
                case RBUS_ERROR_INVALID_METHOD:
                        return WDMP_ERR_METHOD_NOT_SUPPORTED;
                case RBUS_ERROR_TIMEOUT:
                        return WDMP_ERR_TIMEOUT;
                default:
                        return WDMP_FAILURE;
        }
}

/* Map an rbus method invocation result to a JSON-RPC error code for the OPERATE
 * response envelope. */
static int operateMapRbusToJsonRpc(rbusError_t rc)
{
        switch(rc)
        {
                case RBUS_ERROR_INVALID_INPUT:
                        return OPERATE_JSONRPC_INVALID_PARAMS;
                case RBUS_ERROR_INVALID_METHOD:
                case RBUS_ERROR_DESTINATION_NOT_FOUND:
                case RBUS_ERROR_DESTINATION_NOT_REACHABLE:
                case RBUS_ERROR_ELEMENT_DOES_NOT_EXIST:
                        return OPERATE_JSONRPC_METHOD_NOT_FOUND;
                default:
                        return OPERATE_JSONRPC_INTERNAL_ERROR;
        }
}

/*----------------------------------------------------------------------------*/
/*                 WebPA OPERATE asynchronous invocation                      */
/*----------------------------------------------------------------------------*/

/* Pending asynchronous OPERATE requests awaiting their rbusMethod_InvokeAsync
 * callback. The rbus async callback only supplies the method name, so pending
 * entries are correlated to their rspDestination by matching the method name
 * (FIFO when several share a method). */
typedef struct OperateAsyncPending
{
        char *methodName;
        char *rspDestination;
        struct OperateAsyncPending *next;
} OperateAsyncPending;

static OperateAsyncPending *operateAsyncPendingHead = NULL;
static pthread_mutex_t operateAsyncPendingMutex = PTHREAD_MUTEX_INITIALIZER;

/* Deliver an OPERATE result to rspDestination as a WRP event. The event payload
 * mirrors the synchronous SET response shape so both transports carry identical
 * semantics. Ownership of the built payload and source is handed to
 * sendNotification (freed via wrp_free_struct); it copies the destination. */
static void operateSendResultEvent(const char *methodName, const char *rspDestination, const char *encodedMessage, int statusCode)
{
        cJSON *root = NULL;
        cJSON *params = NULL;
        cJSON *entry = NULL;
        char *payload = NULL;
        char *source = NULL;

        if(rspDestination == NULL || strlen(rspDestination) == 0)
        {
                return;
        }

        root = cJSON_CreateObject();
        if(root == NULL)
        {
                return;
        }
        cJSON_AddNumberToObject(root, "statusCode", statusCode);
        cJSON_AddItemToObject(root, "parameters", params = cJSON_CreateArray());
        cJSON_AddItemToArray(params, entry = cJSON_CreateObject());
        cJSON_AddStringToObject(entry, "name", (methodName != NULL) ? methodName : WEBPA_OPERATE_PARAM_NAME);
        cJSON_AddStringToObject(entry, "message", (encodedMessage != NULL) ? encodedMessage : "");
        payload = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        if(payload == NULL)
        {
                return;
        }

        if(strlen(deviceMAC) == 0)
        {
                getDeviceMac();
        }
        source = (char *) malloc(40);
        if(source == NULL)
        {
                free(payload);
                return;
        }
        snprintf(source, 40, "mac:%s", deviceMAC);

        WalInfo("OPERATE async: delivering result for %s to %s\n",
                (methodName != NULL) ? methodName : "NULL", rspDestination);
        sendNotification(payload, source, (char *) rspDestination);
}

/* rbusMethod_InvokeAsync callback: correlates the response to its pending
 * rspDestination, builds the result/error envelope, and delivers it as a WRP
 * event. The rbus worker thread releases inParams and outParams (params) after
 * this returns, so params must only be read here. */
static void operateAsyncCallback(rbusHandle_t handle, char const *methodName, rbusError_t error, rbusObject_t params)
{
        OperateAsyncPending *entry = NULL;
        OperateAsyncPending *prev = NULL;
        OperateAsyncPending *cur = NULL;
        char *encodedMessage = NULL;
        int statusCode = 200;

        (void) handle;

        /* Detach the first pending entry matching this method name. */
        pthread_mutex_lock(&operateAsyncPendingMutex);
        cur = operateAsyncPendingHead;
        while(cur != NULL)
        {
                if(methodName != NULL && cur->methodName != NULL &&
                   strcmp(cur->methodName, methodName) == 0)
                {
                        if(prev == NULL)
                        {
                                operateAsyncPendingHead = cur->next;
                        }
                        else
                        {
                                prev->next = cur->next;
                        }
                        entry = cur;
                        break;
                }
                prev = cur;
                cur = cur->next;
        }
        pthread_mutex_unlock(&operateAsyncPendingMutex);

        if(error == RBUS_ERROR_SUCCESS)
        {
                encodedMessage = operateBuildSuccessMessage(params);
        }
        else
        {
                int code = operateMapRbusToJsonRpc(error);
                char errorData[128];
                errorData[0] = '\0';
                if(code == OPERATE_JSONRPC_INTERNAL_ERROR)
                {
                        snprintf(errorData, sizeof(errorData), "Unexpected rbus failure (rbus error %d)", error);
                }
                encodedMessage = operateBuildErrorMessage(code, (errorData[0] != '\0') ? errorData : NULL);
                statusCode = 520;
        }

        if(entry != NULL)
        {
                operateSendResultEvent(methodName, entry->rspDestination, encodedMessage, statusCode);
                free(entry->methodName);
                free(entry->rspDestination);
                free(entry);
        }
        else
        {
                WalError("OPERATE async: no pending entry for method %s\n", (methodName != NULL) ? methodName : "NULL");
        }

        if(encodedMessage != NULL)
        {
                free(encodedMessage);
        }
}

/* Invoke a method asynchronously and acknowledge immediately. The actual result
 * is delivered later to rspDestination via operateAsyncCallback. This function
 * takes ownership of inParams and always releases it before returning (rbus
 * retains its own reference for the background thread). On success @p result is
 * set to a base64 acknowledgment; on failure it is set to a base64 error
 * envelope. */
static WDMP_STATUS webpaRbusOperateAsync(const char *methodName, const char *rspDestination, rbusObject_t inParams, char **result)
{
        OperateAsyncPending *entry = NULL;
        rbusError_t rc = RBUS_ERROR_BUS_ERROR;

        if(!isRbusInitialized())
        {
                WalError("OPERATE async: rbus not initialized\n");
                if(result != NULL)
                {
                        *result = operateBuildErrorMessage(OPERATE_JSONRPC_INTERNAL_ERROR, "Rbus not initialized");
                }
                if(inParams != NULL)
                {
                        rbusObject_Release(inParams);
                }
                return WDMP_FAILURE;
        }

        entry = (OperateAsyncPending *) malloc(sizeof(OperateAsyncPending));
        if(entry == NULL)
        {
                WalError("OPERATE async: out of memory\n");
                if(result != NULL)
                {
                        *result = operateBuildErrorMessage(OPERATE_JSONRPC_INTERNAL_ERROR, "Out of memory");
                }
                if(inParams != NULL)
                {
                        rbusObject_Release(inParams);
                }
                return WDMP_FAILURE;
        }
        memset(entry, 0, sizeof(OperateAsyncPending));
        entry->methodName = strdup(methodName);
        entry->rspDestination = strdup(rspDestination);

        pthread_mutex_lock(&operateAsyncPendingMutex);
        entry->next = operateAsyncPendingHead;
        operateAsyncPendingHead = entry;
        pthread_mutex_unlock(&operateAsyncPendingMutex);

        WalInfo("OPERATE async: invoking %s, result -> %s\n", methodName, rspDestination);
        rc = rbusMethod_InvokeAsync(rbus_handle, methodName, inParams, operateAsyncCallback, 0);

        /* rbusMethod_InvokeAsync retains its own reference to inParams for the
         * background thread; release our reference regardless of the outcome. */
        if(inParams != NULL)
        {
                rbusObject_Release(inParams);
        }

        if(rc != RBUS_ERROR_SUCCESS)
        {
                WalError("OPERATE async: rbusMethod_InvokeAsync for %s failed with %d\n", methodName, rc);

                /* No callback will fire; detach and free the pending entry. */
                pthread_mutex_lock(&operateAsyncPendingMutex);
                {
                        OperateAsyncPending *prev = NULL;
                        OperateAsyncPending *cur = operateAsyncPendingHead;
                        while(cur != NULL)
                        {
                                if(cur == entry)
                                {
                                        if(prev == NULL)
                                        {
                                                operateAsyncPendingHead = cur->next;
                                        }
                                        else
                                        {
                                                prev->next = cur->next;
                                        }
                                        break;
                                }
                                prev = cur;
                                cur = cur->next;
                        }
                }
                pthread_mutex_unlock(&operateAsyncPendingMutex);

                if(result != NULL)
                {
                        *result = operateBuildErrorMessage(operateMapRbusToJsonRpc(rc), NULL);
                }
                free(entry->methodName);
                free(entry->rspDestination);
                free(entry);
                return WDMP_FAILURE;
        }

        /* Immediate acknowledgment; the real result follows to rspDestination. */
        if(result != NULL)
        {
                cJSON *ack = cJSON_CreateObject();
                if(ack != NULL)
                {
                        cJSON *ackResult = cJSON_CreateObject();
                        cJSON_AddStringToObject(ackResult, "status", "accepted");
                        cJSON_AddItemToObject(ack, "result", ackResult);
                        *result = operateJsonToBase64(ack);
                        cJSON_Delete(ack);
                }
        }
        return WDMP_SUCCESS;
}

WDMP_STATUS webpaRbusOperate(const char *encodedValue, char **methodName, char **result)
{
        WDMP_STATUS status = WDMP_FAILURE;
        size_t decodedLen = 0;
        uint8_t *decoded = NULL;
        cJSON *root = NULL;
        cJSON *methodItem = NULL;
        cJSON *paramsItem = NULL;
        cJSON *parameters = NULL;
        cJSON *rspItem = NULL;
        rbusObject_t inParams = NULL;
        rbusObject_t outParams = NULL;
        rbusError_t rc = RBUS_ERROR_BUS_ERROR;
        int errorCode = 0;
        char errorData[128];

        errorData[0] = '\0';

        if(methodName != NULL)
        {
                *methodName = NULL;
        }
        if(result != NULL)
        {
                *result = NULL;
        }
        if(encodedValue == NULL || result == NULL || strlen(encodedValue) == 0)
        {
                WalError("OPERATE: encoded value or result pointer is NULL/empty\n");
                return WDMP_ERR_INVALID_INPUT_PARAMETER;
        }

        /* Base64-decode the payload. */
        {
                size_t decBufSize = b64_get_decoded_buffer_size(strlen(encodedValue));
                if(decBufSize == 0)
                {
                        decoded = NULL;
                }
                else
                {
                        decoded = (uint8_t *) malloc(decBufSize);
                        if(decoded != NULL)
                        {
                                decodedLen = b64_decode((const uint8_t *) encodedValue, strlen(encodedValue), decoded);
                        }
                }
        }
        if(decoded == NULL || decodedLen == 0)
        {
                WalError("OPERATE: base64 decode failed\n");
                if(decoded != NULL)
                {
                        free(decoded);
                        decoded = NULL;
                }
                status = WDMP_ERR_INVALID_INPUT_PARAMETER;
                errorCode = OPERATE_JSONRPC_PARSE_ERROR;
                goto respond;
        }
        if(decodedLen > WEBPA_OPERATE_MAX_PAYLOAD_SIZE)
        {
                WalError("OPERATE: decoded payload size %zu exceeds max %d\n", decodedLen, WEBPA_OPERATE_MAX_PAYLOAD_SIZE);
                free(decoded);
                decoded = NULL;
                status = WDMP_ERR_INVALID_INPUT_PARAMETER;
                errorCode = OPERATE_JSONRPC_INVALID_REQUEST;
                goto respond;
        }

        /* Parse the decoded bytes as a NUL-terminated JSON string. */
        {
                char *jsonStr = (char *) malloc(decodedLen + 1);
                if(jsonStr == NULL)
                {
                        free(decoded);
                        decoded = NULL;
                        status = WDMP_FAILURE;
                        errorCode = OPERATE_JSONRPC_INTERNAL_ERROR;
                        goto respond;
                }
                memcpy(jsonStr, decoded, decodedLen);
                jsonStr[decodedLen] = '\0';
                root = cJSON_Parse(jsonStr);
                free(jsonStr);
        }
        free(decoded);
        decoded = NULL;

        if(root == NULL)
        {
                WalError("OPERATE: JSON parse failed\n");
                status = WDMP_ERR_INVALID_INPUT_PARAMETER;
                errorCode = OPERATE_JSONRPC_PARSE_ERROR;
                goto respond;
        }

        /* Validate required fields: method (non-empty string) and params (object). */
        methodItem = cJSON_GetObjectItem(root, "method");
        if(methodItem == NULL || !cJSON_IsString(methodItem) || methodItem->valuestring == NULL || strlen(methodItem->valuestring) == 0)
        {
                WalError("OPERATE: missing or empty 'method'\n");
                status = WDMP_ERR_INVALID_INPUT_PARAMETER;
                errorCode = OPERATE_JSONRPC_INVALID_REQUEST;
                goto respond;
        }
        /* Capture the invoked method name so the response 'name' can echo it. */
        if(methodName != NULL)
        {
                *methodName = strdup(methodItem->valuestring);
        }

        paramsItem = cJSON_GetObjectItem(root, "params");
        if(paramsItem == NULL || !cJSON_IsObject(paramsItem))
        {
                WalError("OPERATE: missing or invalid 'params'\n");
                status = WDMP_ERR_INVALID_INPUT_PARAMETER;
                errorCode = OPERATE_JSONRPC_INVALID_REQUEST;
                goto respond;
        }

        /* params.parameters is the flattened key/value set (optional). */
        parameters = cJSON_GetObjectItem(paramsItem, "parameters");

        /* Build flat inParams. */
        status = buildOperateInParams(parameters, &inParams);
        if(status != WDMP_SUCCESS)
        {
                WalError("OPERATE: failed to build inParams\n");
                errorCode = OPERATE_JSONRPC_INTERNAL_ERROR;
                snprintf(errorData, sizeof(errorData), "Conversion from JSON parameters to RBUS parameters failed");
                goto respond;
        }

        /* An optional rspDestination selects asynchronous invocation: acknowledge
         * immediately and deliver the result later as a WRP event. */
        rspItem = cJSON_GetObjectItem(root, "rspDestination");
        if(rspItem != NULL && cJSON_IsString(rspItem) && rspItem->valuestring != NULL && strlen(rspItem->valuestring) > 0)
        {
                /* webpaRbusOperateAsync takes ownership of inParams. */
                status = webpaRbusOperateAsync(methodItem->valuestring, rspItem->valuestring, inParams, result);
                inParams = NULL;
                cJSON_Delete(root);
                return status;
        }

        /* Invoke the method synchronously. */
        rc = webpaRbusMethodInvoke(methodItem->valuestring, inParams, &outParams);
        status = mapRbusToWdmpStatus(rc);
        if(rc != RBUS_ERROR_SUCCESS)
        {
                WalError("OPERATE: rbusMethod_Invoke for %s failed with %d\n", methodItem->valuestring, rc);
                errorCode = operateMapRbusToJsonRpc(rc);
                if(errorCode == OPERATE_JSONRPC_INTERNAL_ERROR)
                {
                        snprintf(errorData, sizeof(errorData), "Unexpected rbus failure (rbus error %d)", rc);
                }
                goto respond;
        }

        /* Success: encode {"result": <outParams>} as base64. */
        *result = operateBuildSuccessMessage(outParams);
        if(*result == NULL)
        {
                WalError("OPERATE: failed to encode result\n");
                status = WDMP_FAILURE;
                errorCode = OPERATE_JSONRPC_INTERNAL_ERROR;
                snprintf(errorData, sizeof(errorData), "Conversion from RBUS result object to JSON result object failed");
                goto respond;
        }

        if(inParams != NULL)
        {
                rbusObject_Release(inParams);
        }
        if(outParams != NULL)
        {
                rbusObject_Release(outParams);
        }
        cJSON_Delete(root);
        return WDMP_SUCCESS;

respond:
        /* Any failure path lands here: build the base64 error envelope so the
         * caller can return it inline (sync) or via the rspDestination event
         * (async). */
        if(result != NULL && *result == NULL && errorCode != 0)
        {
                *result = operateBuildErrorMessage(errorCode, (errorData[0] != '\0') ? errorData : NULL);
        }
        if(inParams != NULL)
        {
                rbusObject_Release(inParams);
        }
        if(outParams != NULL)
        {
                rbusObject_Release(outParams);
        }
        if(root != NULL)
        {
                cJSON_Delete(root);
        }
        if(status == WDMP_SUCCESS)
        {
                status = WDMP_FAILURE;
        }
        return status;
}
