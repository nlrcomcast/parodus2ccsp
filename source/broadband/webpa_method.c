/**
 * @file webpa_method.c
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2025  Comcast
 */

#include "webpa_method.h"
#include <string.h>

/**
 *@brief  Validate a METHOD request structure.
 *
 * @param methodReq [in] Pointer to method request structure.
 * @return WDMP_SUCCESS (0) if valid, WDMP_FAILURE (1) if invalid.
 */
int validate_method_req(method_req_t *methodReq)
{
    if (methodReq == NULL)
    {
        WalError("validate_method_req: methodReq is NULL\n");
        return WDMP_FAILURE;
    }

    if (methodReq->methodName == NULL || strlen(methodReq->methodName) == 0)
    {
        WalError("validate_method_req: methodName is NULL or empty\n");
        return WDMP_FAILURE;
    }

    size_t len = strlen(methodReq->methodName);
    if (len < 2 || strcmp(&(methodReq->methodName[len - 2]), "()") != 0)
    {
        WalError("validate_method_req: methodName must end with '()' — got '%s'\n", methodReq->methodName);
        return WDMP_FAILURE;
    }

    // Validate each parameter object
    for (size_t i = 0; i < methodReq->objectCnt; i++)
    {
        method_param_t *obj = &methodReq->objects[i];

        if (obj->paramCnt == 0 || obj->params == NULL)
        {
            WalError("validate_method_req: object[%zu] has no params\n", i);
            return WDMP_FAILURE;
        }

        for (size_t j = 0; j < obj->paramCnt; j++)
        {
            param_t *param = &obj->params[j];

            if (param->name == NULL || strlen(param->name) == 0)
            {
                WalError("validate_method_req: object[%zu].param[%zu] name is NULL or empty\n", i, j);
                return WDMP_FAILURE;
            }

            if (param->value == NULL || strlen(param->value) == 0)
            {
                WalError("validate_method_req: object[%zu].param[%zu] value is NULL or empty\n", i, j);
                return WDMP_FAILURE;
            }
        }
    }

    WalInfo("validate_method_req: validation successful for method '%s'\n", methodReq->methodName);
    return WDMP_SUCCESS;
}

void invokeMethod(method_req_t *methodReq, res_struct *resObj)
{
    // rbusHandle_t rbus_handle = NULL;
    // rbusObject_t inParams = NULL, outParams = NULL;
    // rbusError_t rc;
    WalInfo("Invoking RBUS method: %s\n", methodReq->methodName);

    // rbus_handle = get_webpa_rbus_Handle();
    // if (!rbus_handle)
    // {
    //     WalError("rbus_methodHandler failed: rbus_handle is NULL\n");
    //     return;
    // }

    // // Initialize inParams and fill with key-value pairs (excluding "Method")

    // for(int i=0; i < methodReq->objectCnt; i++)
    // {
    //     method_param_t *obj = &methodReq->objects[i];
    //     rbusObject_Init(&inParams, "NULL");
    //     for(int j=0; j<obj->paramCnt; j++)
    //     {
	// 				rbusValue_t val;
	// 				rbusValue_Init(&val);

    //                 if(obj->params[j].type == WDMP_STRING)
    //                     rbusValue_SetString(val, obj->params[j].value);
    //                 if(obj->params[j].type == WDMP_INT)
    //                     rbusValue_SetDouble(val, obj->params[j].value);
    //                 if(obj->params[j].type == WDMP_BOOLEAN)
    //                     rbusValue_SetBoolean(val, obj->params[j].value);
                                                
	// 				rbusObject_SetValue(inParams, obj->params[j].name, val);
	// 				rbusValue_Release(val);
    //     }
    // }
    // // Call the RBUS method
    // rc = rbusMethod_Invoke(rbus_handle, methodReq->methodName, inParams, &outParams);
    // rbusObject_Release(inParams);

	// if(rc != RBUS_ERROR_SUCCESS)
	// 	ParodusError("rbusMethod_Invoke failed for %s. ret: %d %s\n", methodReq->methodName, rc, rbusError_ToString(rc));
	// else
	// 	ParodusInfo("rbusMethod_Invoke success. ret: %d %s\n", rc, rbusError_ToString(rc));    
}