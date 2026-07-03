#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include <cJSON.h>
#include <trower-base64/base64.h>
#include "webpa_rbus.h"

static rbusHandle_t rbus_handle;
static bool isRbus = false;

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

WDMP_STATUS webpaRbusOperate(const char *encodedValue, char **result)
{
        WDMP_STATUS status = WDMP_FAILURE;
        size_t decodedLen = 0;
        uint8_t *decoded = NULL;
        cJSON *root = NULL;
        cJSON *methodItem = NULL;
        cJSON *paramsItem = NULL;
        cJSON *parameters = NULL;
        const char *methodName = NULL;
        rbusObject_t inParams = NULL;
        rbusObject_t outParams = NULL;
        rbusError_t rc = RBUS_ERROR_BUS_ERROR;
        cJSON *resultJson = NULL;
        char *resultStr = NULL;

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
        decoded = b64_decode_with_alloc((const uint8_t *) encodedValue, strlen(encodedValue), &decodedLen);
        if(decoded == NULL || decodedLen == 0)
        {
                WalError("OPERATE: base64 decode failed\n");
                if(decoded != NULL)
                {
                        free(decoded);
                }
                return WDMP_ERR_INVALID_INPUT_PARAMETER;
        }
        if(decodedLen > WEBPA_OPERATE_MAX_PAYLOAD_SIZE)
        {
                WalError("OPERATE: decoded payload size %zu exceeds max %d\n", decodedLen, WEBPA_OPERATE_MAX_PAYLOAD_SIZE);
                free(decoded);
                return WDMP_ERR_INVALID_INPUT_PARAMETER;
        }

        /* Parse the decoded bytes as a NUL-terminated JSON string. */
        {
                char *jsonStr = (char *) malloc(decodedLen + 1);
                if(jsonStr == NULL)
                {
                        free(decoded);
                        return WDMP_FAILURE;
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
                return WDMP_ERR_INVALID_INPUT_PARAMETER;
        }

        /* Validate required fields: method (non-empty string) and params (object). */
        methodItem = cJSON_GetObjectItem(root, "method");
        if(methodItem == NULL || !cJSON_IsString(methodItem) || methodItem->valuestring == NULL || strlen(methodItem->valuestring) == 0)
        {
                WalError("OPERATE: missing or empty 'method'\n");
                cJSON_Delete(root);
                return WDMP_ERR_INVALID_INPUT_PARAMETER;
        }
        methodName = methodItem->valuestring;

        paramsItem = cJSON_GetObjectItem(root, "params");
        if(paramsItem == NULL || !cJSON_IsObject(paramsItem))
        {
                WalError("OPERATE: missing or invalid 'params'\n");
                cJSON_Delete(root);
                return WDMP_ERR_INVALID_INPUT_PARAMETER;
        }

        /* params.parameters is the flattened key/value set (optional). */
        parameters = cJSON_GetObjectItem(paramsItem, "parameters");

        /* Build flat inParams. */
        status = buildOperateInParams(parameters, &inParams);
        if(status != WDMP_SUCCESS)
        {
                WalError("OPERATE: failed to build inParams\n");
                cJSON_Delete(root);
                return status;
        }

        /* Invoke the method synchronously. */
        rc = webpaRbusMethodInvoke(methodName, inParams, &outParams);
        status = mapRbusToWdmpStatus(rc);
        if(rc != RBUS_ERROR_SUCCESS)
        {
                WalError("OPERATE: rbusMethod_Invoke for %s failed with %d\n", methodName, rc);
        }
        else
        {
                /* Serialize outParams -> JSON -> base64. */
                resultJson = operateOutParamsToJson(outParams);
                if(resultJson != NULL)
                {
                        char *jsonText = cJSON_PrintUnformatted(resultJson);
                        if(jsonText != NULL)
                        {
                                size_t encLen = 0;
                                resultStr = b64_encode_with_alloc((const uint8_t *) jsonText, strlen(jsonText), &encLen);
                                free(jsonText);
                        }
                        cJSON_Delete(resultJson);
                }
                if(resultStr == NULL)
                {
                        WalError("OPERATE: failed to encode result\n");
                        status = WDMP_FAILURE;
                }
                else
                {
                        *result = resultStr;
                }
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
        return status;
}
