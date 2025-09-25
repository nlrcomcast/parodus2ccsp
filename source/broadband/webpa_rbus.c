#include <stdbool.h>
#include <string.h>

#include <stdlib.h>
#include <wdmp-c.h>
#include <cimplog.h>
#include "webpa_rbus.h"
#include "webpa_notification.h"

static rbusHandle_t rbus_handle;
static bool isRbus = false;

extern PARAMVAL_CHANGE_SOURCE mapWriteID(unsigned int writeID);

rbusDataElement_t dataElements[2] = {
    {WEBPA_NOTIFY_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NotifyParamGetHandler, NULL, NULL, NULL, NULL, NULL}},
    {WEBPA_SUBSCRIBE_LIST, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, SubscribeNotifyParamMethodHandler}}
};

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
        if(isRbusInitialized)
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
	if(isRbusInitialized)
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
	if(isRbusInitialized)
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
}

/**
 * Register data elements for data model and methods implementation using rbus.
 */
int regWebPaDataModel()
{
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    if(!rbus_handle)
    {
        WalError("regWebPaDataModel failed in getting bus handles\n");
        return rc;
    }

	rc = rbus_regDataElements(rbus_handle, 2, dataElements);

    if(rc == RBUS_ERROR_SUCCESS)
    {
		WalInfo("Registered data element %s with rbus \n ", WEBPA_NOTIFY_PARAM);
    }
    else
	{
		WalError("Failed in registering data element %s \n", WEBPA_NOTIFY_PARAM);
	}
	return rc;
}

/**
 * Un-Register data elements for dataModel implementation using rbus.
 */
int UnregWebPaDataModel()
{
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    if(!rbus_handle)
    {
        WalError("regWebPaDataModel failed in getting bus handles\n");
        return rc;
    }

	rc = rbus_unregDataElements(rbus_handle, 2, dataElements);
    if(rc == RBUS_ERROR_SUCCESS)
    {
		WalInfo("Registered data element %s with rbus \n ", WEBPA_NOTIFY_PARAM);
    }
    else
	{
		WalError("Failed in registering data element %s \n", WEBPA_NOTIFY_PARAM);
	}
	return rc;
}

rbusError_t NotifyParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;
    WalInfo("NotifyParamGetHandler is called\n");
    const char* paramName = rbusProperty_GetName(property);
    if(strncmp(paramName, WEBPA_NOTIFY_PARAM, strlen(WEBPA_NOTIFY_PARAM)) != 0)
    {
        WalError("Unexpected parameter = %s\n", paramName);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    char** params = getGlobalNotifyParams();
    if(!params)
        return RBUS_ERROR_BUS_ERROR;

    size_t totalLen = 1;
    for (int i = 0; params[i] != NULL; i++)
    {
        totalLen += strlen(params[i]);
        if (params[i+1] != NULL)
            totalLen += 1;
    }

    char* buffer = malloc(totalLen);
    if(!buffer)
        return RBUS_ERROR_BUS_ERROR;

    char* ptr = buffer;
    for(int i = 0; params[i] != NULL; i++)
    {
        size_t paramLen = strlen(params[i]);
        memcpy(ptr, params[i], paramLen);
        ptr += paramLen;
        
        if(params[i+1] != NULL)
        {
            *ptr++ = '\n';
        }
    }
    *ptr = '\0';

    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, buffer);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    free(buffer);
    return RBUS_ERROR_SUCCESS;
}

static void eventReceiveHandler(
    rbusHandle_t rbus_handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)rbus_handle;
    (void)subscription;

    rbusValue_t newValue, oldValue, writeIdValue;
    ParamNotify *paramNotify;
    char newValueStr[256] = "NULL";
    char oldValueStr[256] = "NULL";
    char writeIdStr[256]  = "NULL";

    unsigned int writeID = DSLH_MPA_ACCESS_CONTROL_CLI;

    if (!event || !event->data || !event->name)
    {
        WalError("Received null event or event data\n");
        return;
    }

    newValue = rbusObject_GetValue(event->data, "value");
    oldValue = rbusObject_GetValue(event->data, "oldValue");
    writeIdValue = rbusObject_GetValue(event->data, "by");

    if(newValue)
        rbusValue_ToString(newValue, newValueStr, sizeof(newValueStr));
    if(oldValue)
        rbusValue_ToString(oldValue, oldValueStr, sizeof(oldValueStr));
    if(writeIdValue)
        rbusValue_ToString(writeIdValue, writeIdStr, sizeof(writeIdStr));

    paramNotify= (ParamNotify *) malloc(sizeof(ParamNotify));

    if(event->name != NULL)
		paramNotify->paramName = strdup(event->name);
    if(newValueStr != NULL)
		paramNotify->newValue = strdup(newValueStr);
	if(oldValueStr != NULL)
		paramNotify->oldValue = strdup(oldValueStr);
    writeID = string_to_writeid(writeIdStr);
	paramNotify->changeSource = mapWriteID(writeID);

	WalInfo("Notification Event from rbus: Parameter Name: %s, Old Value: %s, New Value: %s, Change Source: %u (%s)\n",
            paramNotify->paramName,
            paramNotify->oldValue ? paramNotify->oldValue : "NULL",
            paramNotify->newValue ? paramNotify->newValue : "NULL",
            paramNotify->changeSource,
            writeIdStr ? writeIdStr : "NULL"
        );

    return;
}

static void subscribeAsyncHandler(
    rbusHandle_t rbus_handle,
    rbusEventSubscription_t* subscription,
    rbusError_t error)
{
    (void)rbus_handle;
    WalInfo("subscribeAsyncHandler event %s, error %d - %s\n", subscription->eventName, error, rbusError_ToString(error));
}

void subscribeToNotifyParams(
    const char* params[], int paramCount,
    char*** succeededParams, int* successCount,
    char*** failedParams, int* failureCount)
{
    WalInfo("Inside subscribeToNotifyParams\n");

    if (succeededParams) *succeededParams = calloc(paramCount, sizeof(char*));
    if (failedParams) *failedParams    = calloc(paramCount, sizeof(char*));
    if (successCount) *successCount = 0;
    if (failureCount) *failureCount = 0;

    if ((succeededParams && !*succeededParams) || (failedParams && !*failedParams))
    {
        WalError("Memory allocation failed\n");
        if (succeededParams) free(*succeededParams);
        if (failedParams) free(*failedParams);
        return;
    }

    for (int i = 0; i < paramCount; i++)
    {
        const char* param = params[i];
        if (!param || !*param)
        {
            WalError("Invalid parameter: %s\n", param ? param : "NULL");
            if (failedParams && failureCount)
            {
                (*failedParams)[(*failureCount)++] = strdup(param);
            }
            continue;
        }
        rbusError_t err = rbusEvent_SubscribeAsync(
            rbus_handle,
            param,
            eventReceiveHandler,
            subscribeAsyncHandler,
            "webpa_notify",
            0);

        if (err != RBUS_ERROR_SUCCESS)
        {
            WalError("Subscribe failed for %s : %u (%s)\n", param, err,
                    (err == RBUS_ERROR_SUBSCRIPTION_ALREADY_EXIST) ? "Subscription Already Exists" : rbusError_ToString(err) );           
            if (failedParams && failureCount)
            {
                (*failedParams)[(*failureCount)++] = strdup(param);
            }
        }
        else
        {
            if (succeededParams && successCount)
            {
                (*succeededParams)[(*successCount)++] = strdup(param);
            }
        }
    }
    return;
}

rbusError_t SubscribeNotifyParamMethodHandler(
    rbusHandle_t handle,
    const char* methodName,
    rbusObject_t inParams,
    rbusObject_t outParams,
    rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)methodName;
    (void)asyncHandle;

    WalInfo("SubscribeNotifyParamMethodHandler invoked\n");

    char* msgBuf = NULL;
    char* successBuf = NULL;
    char* failedBuf = NULL;
    rbusValue_t message, statusCode;
    rbusValue_Init(&message);
    rbusValue_Init(&statusCode);

/* Extract inParams */
    rbusProperty_t props = rbusObject_GetProperties(inParams);
    int count = props ? rbusProperty_Count(props) : 0;

    if (count == 0)
    {
        WalError("No parameters provided\n");
        rbusValue_SetString(message, "No parameters provided");
        rbusValue_SetInt32(statusCode, 400);
        goto set_response_and_return;
    }

    WalInfo("Total subscriptions: %d\n", count);

    // Extract parameters
    const char** dynamicNotifyParams = calloc(count, sizeof(char*));
    if (!dynamicNotifyParams)
    {
        WalError("Memory allocation failed for param array\n");
        rbusValue_SetString(message, "Memory allocation failed");
        rbusValue_SetInt32(statusCode, 500);
        goto set_response_and_return;
    }

    int validCount = 0;
    for (int i = 0; i < count; i++)
    {
        char keyName[64];
        rbusObject_t subObj = NULL;
        snprintf(keyName, sizeof(keyName), "param%d", i);

        rbusValue_t paramVal = rbusObject_GetValue(inParams, keyName);
        if (paramVal && rbusValue_GetType(paramVal) == RBUS_OBJECT)
            subObj = rbusValue_GetObject(paramVal);

        if (!subObj)
        {
            WalError("%s missing object\n", keyName);
            continue;
        }

        const char* name = NULL;
        const char* notifType = NULL;
        bool notifRetry = false;
        int notifExpiration = 0;

        rbusValue_t val = NULL;

        val = rbusObject_GetValue(subObj, "name");
        if (val && rbusValue_GetType(val) == RBUS_STRING)
            name = rbusValue_GetString(val, NULL);

        val = rbusObject_GetValue(subObj, "notificationType");
        if (val && rbusValue_GetType(val) == RBUS_STRING)
            notifType = rbusValue_GetString(val, NULL);

        val = rbusObject_GetValue(subObj, "notifRetry");
        if (val && rbusValue_GetType(val) == RBUS_BOOLEAN)
            notifRetry = rbusValue_GetBoolean(val);

        val = rbusObject_GetValue(subObj, "notifExpiration");
        if (val && (rbusValue_GetType(val) == RBUS_INT32 || rbusValue_GetType(val) == RBUS_INT64))
            notifExpiration = rbusValue_GetInt32(val);

        WalInfo("%s: name=%s, notificationType=%s, notifRetry=%s, notifExpiration=%d\n", keyName, name ? name : "NULL", notifType ? notifType : "NULL", notifRetry ? "true" : "false", notifExpiration);

        if (!name || !*name)
        {
            WalError("%s missing 'name'\n", keyName);
            continue;
        }

        if (notifType && strcmp(notifType, "ValueChange") == 0)
        {
            dynamicNotifyParams[validCount++] = name;
        }
        else
        {
            WalInfo("Skipping %s: unsupported notificationType=%s\n",
                    name, notifType ? notifType : "NULL");
        }
    }

    if (validCount == 0)
    {
        WalError("No valid parameters found\n");
        free(dynamicNotifyParams);
        rbusValue_SetString(message, "No valid parameters provided");
        rbusValue_SetInt32(statusCode, 400);
        goto set_response_and_return;
    }

/* Add new dynamic lists to FILE */
setGlobalNotifyParams(dynamicNotifyParams, validCount, UPDATE_LIST_AND_WRITE_FILE);

/* Subscribe to parameters via rbus */
    char** succeededParams = NULL;
    char** failedParams = NULL;
    int successCount = 0, failureCount = 0;

    subscribeToNotifyParams(dynamicNotifyParams, validCount,&succeededParams, &successCount, &failedParams, &failureCount);
    free(dynamicNotifyParams);

    /* Build comma-separated success/failure lists */
    size_t successBufLen = 1, failedBufLen = 1;
    for (int i = 0; i < successCount; i++) successBufLen += strlen(succeededParams[i]) + 2; /* ", " */
    for (int i = 0; i < failureCount; i++) failedBufLen  += strlen(failedParams[i])   + 2;

    successBuf = calloc(successBufLen, 1);
    failedBuf  = calloc(failedBufLen, 1);
    if (!successBuf || !failedBuf)
    {
        WalError("Failed to allocate successBuf or failedBuf\n");
        rbusValue_SetString(message, "Memory allocation failed");
        rbusValue_SetInt32(statusCode, 500);
        goto set_response_and_return;
    }

    size_t successLen = 0, failedLen = 0;
    for (int i = 0; i < successCount; i++)
    {
        int wrote = snprintf(successBuf + successLen, successBufLen - successLen, "%s%s",
                             (i > 0) ? ", " : "", succeededParams[i]);
        if (wrote < 0) break;
        successLen += (size_t)wrote;
    }
    for (int i = 0; i < failureCount; i++)
    {
        int wrote = snprintf(failedBuf + failedLen, failedBufLen - failedLen, "%s%s",
                             (i > 0) ? ", " : "", failedParams[i]);
        if (wrote < 0) break;
        failedLen += (size_t)wrote;
    }

    /* Format final response message */
    size_t msgBuf_len = successLen + failedLen + 256;
    msgBuf = malloc(msgBuf_len);
    if (!msgBuf)
    {
        WalError("Failed to allocate msgBuf\n");
        rbusValue_SetString(message, "Memory allocation failed");
        rbusValue_SetInt32(statusCode, 500);
        goto set_response_and_return;
    }

    if (failureCount == 0)
        snprintf(msgBuf, msgBuf_len, "subscription requests initiated successfully");
    else if(successCount == 0)
        snprintf(msgBuf, msgBuf_len, "subscription requests failed");
    else
        snprintf(msgBuf, msgBuf_len, "Partial success: %d succeeded [%s], %d failed [%s]", successCount, successBuf, failureCount, failedBuf);

    /* Set verbose output message to rbus outParams */
    rbusValue_SetString(message, msgBuf);
    rbusValue_SetInt32(statusCode, failureCount == 0 ? 200 : 207);

set_response_and_return:
    rbusObject_SetValue(outParams, "message", message);
    rbusObject_SetValue(outParams, "statusCode", statusCode);

    WalInfo("SubscribeNotifyParamMethodHandler completed: %s\n", msgBuf ? msgBuf : "SUBSCRIPTION STATUS UNKNOWN");

    if (message) rbusValue_Release(message);
    if (statusCode) rbusValue_Release(statusCode);

    if (succeededParams)
    {
        for (int i = 0; i < successCount; i++) free(succeededParams[i]);
        free(succeededParams);
    }
    if (failedParams)
    {
        for (int i = 0; i < failureCount; i++) free(failedParams[i]);
        free(failedParams);
    }

    if (successBuf) free(successBuf);
    if (failedBuf) free(failedBuf);
    if (msgBuf) free(msgBuf);
    msgBuf = NULL;

    return RBUS_ERROR_SUCCESS;
}
