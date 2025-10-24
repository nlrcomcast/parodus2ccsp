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

#include <dslh_definitions_database.h>

#define MAX_PARAM_LEN 256
#define WEBPA_NOTIFY_PARAM "Device.DeviceInfo.Webpa.NotifySubscriptionList"
#define WEBPA_NOTIFY_SUBSCRIPTION "Device.Webpa.Subscription.NotifyEvent()"

typedef enum {
    NOTIFY_SUBSCRIPTION_SUCCESS                 =  200,
    NOTIFY_SUBSCRIPTION_FAILURE                 =  500,
    NOTIFY_SUBSCRIPTION_INVALID_INPUT           =  400,
    NOTIFY_SUBSCRIPTION_ALREADY_EXISTS          =  409,
    NOTIFY_SUBSCRIPTION_BOOTUP_IN_PROGRESS      =  503,
    NOTIFY_SUBSCRIPTION_MULTI_STATUS            =  207
} NOTIFY_SUBSCRIPTION_STATUS_CODE;

bool isRbusEnabled();
bool isRbusInitialized();
WDMP_STATUS webpaRbusInit(const char *pComponentName);
void webpaRbus_Uninit();
rbusError_t setTraceContext(char* traceContext[]);
rbusError_t getTraceContext(char* traceContext[]);
rbusError_t clearTraceContext();
void regWebPaDataModel();
rbusError_t NotifyParamGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
rbusError_t NotifyParamMethodHandler(rbusHandle_t handle, const char* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle);

#endif