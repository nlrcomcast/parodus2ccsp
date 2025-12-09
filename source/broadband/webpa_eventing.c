/**
 * @file webpa_eventing.c
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2025  Comcast
 */
#include <unistd.h>
#include <cJSON.h>
#include <string.h>
#include "webpa_eventing.h"
#include "webpa_adapter.h"

g_NotifyParam *g_NotifyParamHead = NULL;
g_NotifyParam *g_NotifyParamTail = NULL;
pthread_mutex_t g_NotifyParamMut = PTHREAD_MUTEX_INITIALIZER;

bool bootupNotifyInitDone = false; //By default set to false until initial notify ON is done minimum one iteration

bool getBootupNotifyInitDone()
{
    return bootupNotifyInitDone;
}

void setBootupNotifyInitDone(bool value)
{
   bootupNotifyInitDone  = value;
}

void addParamToGlobalList(const char *paramName,bool paramType, bool paramSubscriptionStatus)
{
	if (!paramName)
    {
        WalError("Parameter name is NULL while adding into addParamToGlobalList\n");
        return;
    }

	g_NotifyParam *node = (g_NotifyParam *)malloc(sizeof(g_NotifyParam));
	if(node == NULL)
	{
		WalError("g_NotifyParam Memory allocation failed\n");
		return;
	}

	memset(node, 0, sizeof(g_NotifyParam));
	node->paramName = strdup(paramName);
	node->paramType = paramType;
	node->paramSubscriptionStatus = paramSubscriptionStatus;
	node->next = NULL;

    pthread_mutex_lock(&g_NotifyParamMut);
	if (g_NotifyParamHead == NULL)
    {
        g_NotifyParamHead = node;
        g_NotifyParamTail = node;
    }
    else
    {
        g_NotifyParamTail->next = node;
        g_NotifyParamTail = node;
    }
	pthread_mutex_unlock(&g_NotifyParamMut);    
}

// Caller must free the returned copy
g_NotifyParam* searchParaminGlobalList(const char *paramName)
{
	if (!paramName)
    {
        WalError("Parameter name is NULL while searching in GlobalList\n");
        return NULL;
    }
    g_NotifyParam *temp = NULL;
    pthread_mutex_lock(&g_NotifyParamMut);
	temp = g_NotifyParamHead;
	while(temp != NULL)
	{
		if(strcmp(temp->paramName,paramName) == 0)
		{
            WalPrint("Parameter %s is found in list\n",paramName);
            break;
		}
		temp = temp->next;
	}
    pthread_mutex_unlock(&g_NotifyParamMut);

	return temp;
}

g_NotifyParam* getGlobalNotifyHead()
{
    pthread_mutex_lock(&g_NotifyParamMut);
    g_NotifyParam *head = g_NotifyParamHead;
    pthread_mutex_unlock(&g_NotifyParamMut);
    return head;
}

int writeDynamicParamToDBFile(const char *param)
{
	FILE *fp;
	fp = fopen(NOTIFY_PARAM_FILE , "a");
	if (fp == NULL)
	{
		WalError("Failed to open file for write %s\n", NOTIFY_PARAM_FILE);
		return 0;
	}
	if(param !=NULL)
	{
		fprintf(fp,"%s\n",param);
		fclose(fp);
		return 1;
	}
	else
	{
		WalError("writeToDBFile failed, param is NULL\n");
		fclose(fp);
		return 0;
	}
}

void readDynamicParamsFromDBFile(int *notifyListSize)
{
	FILE *fp;
	char param[512];
	
	WalInfo("Dynamic parameters reading from DB %s\n",NOTIFY_PARAM_FILE);

	if (access(NOTIFY_PARAM_FILE, F_OK) != 0)
	{
		WalInfo("No dynamic parameters were available for this device\n");
		return ;
	}

	fp = fopen(NOTIFY_PARAM_FILE , "r");
	if (fp == NULL)
	{
		WalError("Failed to open file in db '%s' for read\n", NOTIFY_PARAM_FILE);
		return ;
	}

	// Read each line until EOF
	while (fscanf(fp,"%511s", param) != EOF) 
    {
        addParamToGlobalList(param,DYNAMIC_PARAM,OFF);
        (*notifyListSize)++;
	}

	fclose(fp);
	WalInfo("Successfully read params from %s\n", NOTIFY_PARAM_FILE);
    return ; 
}

char* CreateJsonFromGlobalNotifyList()
{
	char *paramList = NULL;

	g_NotifyParam *temp = getGlobalNotifyHead();
	cJSON *jsonArray = cJSON_CreateArray();
    while (temp != NULL) 
	{
        // Local copies for safe access outside lock
        char *paramName = NULL;
        bool paramType = false;
        bool status = false;
        g_NotifyParam *next = NULL;

        // Copy under lock
        pthread_mutex_lock(&g_NotifyParamMut);
        paramName = strdup(temp->paramName);
        paramType = temp->paramType;
        status = temp->paramSubscriptionStatus;
        next = temp->next;
        pthread_mutex_unlock(&g_NotifyParamMut);

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ParamName", paramName);
        cJSON_AddStringToObject(item, "Type", paramType?"Static":"Dynamic");
        cJSON_AddStringToObject(item, "Status", status?"ON":"OFF");
        cJSON_AddItemToArray(jsonArray, item);

        WAL_FREE(paramName);
        temp = next;
    }
	paramList = cJSON_PrintUnformatted(jsonArray);
	cJSON_Delete(jsonArray);
	return paramList;	
}

bool getParamStatus(g_NotifyParam *param) 
{
    bool status;
    pthread_mutex_lock(&g_NotifyParamMut);
    status = param->paramSubscriptionStatus;
    pthread_mutex_unlock(&g_NotifyParamMut);
    return status;
}

// Function to safely update paramSubscriptionStatus
void updateParamStatus(g_NotifyParam *param, bool status) 
{
    if (param == NULL) 
        return;

    pthread_mutex_lock(&g_NotifyParamMut);
    param->paramSubscriptionStatus = status;
    pthread_mutex_unlock(&g_NotifyParamMut);
}

int validateDynamicEventingData(method_req_t *methodReq, char **errorMsg)
{
	if(methodReq->objectCnt == 0)
	{
		*errorMsg = strdup("Invalid method request format");
		return WDMP_ERR_NOTIF_FIELD_COUNT;
	}
    for (size_t i = 0; i < methodReq->objectCnt; i++)
    {
        method_param_t *obj = &methodReq->objects[i];
		    // Ensure we have at least 2 parameters
		if (!obj->params || obj->paramCnt < 2) 
		{
			*errorMsg = strdup("Insufficient parameters");
			WalError("Insufficient parameters\n");
			return WDMP_ERR_NOTIF_FIELD_COUNT;
		}

		kv_pair_t *param = &obj->params[0];
		// Validate 'name' key
		if((param->name == NULL) || (strcmp(param->name, "name") != 0))
		{
			*errorMsg = strdup("Missing name field in method request");
			WalError("Missing name field in method request\n");
			return WDMP_ERR_NOTIF_NAME_FIELD;
		}
		// Validate 'name' key
		if((param->type != WDMP_STRING) || (param->value.s == NULL))
		{
			*errorMsg = strdup("Missing parameter name in method request");
			WalError("Missing parameter name in method request\n");
			return WDMP_ERR_NOTIF_NAME_MISSING;
		}

		param = &obj->params[1];
		// Validate 'notificationType' key
		if(!param->name || strcmp(param->name, "notificationType") != 0)
		{
			*errorMsg = strdup("Missing notificationType field in method request");
			WalError("Missing notificationType field in method request\n");
			return WDMP_ERR_NOTIF_TYPE_FIELD;
		}
		// Validate 'notificationType' value
		if((param->type != WDMP_STRING) || (param->value.s == NULL))
		{
			*errorMsg = strdup("Missing parameter notificationType field in method request");
			WalError("Missing parameter notificationType field in method request\n");
			return WDMP_ERR_NOTIF_TYPE_MISSING;
		}		
		// Validate 'notificationType' value
		if(strcmp(param->value.s, "ValueChange") != 0)
		{
			*errorMsg = strdup("Notification type is not supported");
			WalError("Notification type is not supported:%s\n",param->value.s);
			return WDMP_ERR_NOTIF_TYPE_INVALID;
		}
	}
	return WDMP_SUCCESS;
}

void ProcessNotifyParamMethod(method_req_t *methodReq, res_struct *resObj)
{
	char *errorMsg = NULL;
	char *paramStatus = NULL;
	bool failure = false;

    if (methodReq == NULL)
    {
        WalError("ProcessNotifyParamMethod: methodReq is NULL\n");
        return;
    }
    param_t att;
	WDMP_STATUS wret = WDMP_FAILURE;
	memset(&att, 0, sizeof(att));

	wret = validateDynamicEventingData(methodReq,&errorMsg);
	if(wret != WDMP_SUCCESS)
	{
        resObj->u.methodRes->statusCode = wret;
        resObj->u.methodRes->message = strdup(errorMsg);		
		WalError("validateDynamicEventingData failed for method '%s' \n", methodReq->methodName);
		WAL_FREE(errorMsg);
		return;
	}

	paramStatus = calloc(methodReq->objectCnt, sizeof(char));
	if (!paramStatus)
	{
		resObj->u.methodRes->statusCode = WDMP_FAILURE;
		resObj->u.methodRes->message = strdup("Out of memory");
		WalError("ProcessNotifyParamMethod: Failed to allocate memory for paramStatus array\n");		
		return;
	}
    // Process each parameter object
    for (size_t i = 0; i < methodReq->objectCnt; i++)
    {
        method_param_t *obj = &methodReq->objects[i];
		kv_pair_t *param = &obj->params[0];
		WalInfo("Processing parmaeter %s for subscription\n",param->value.s);

		g_NotifyParam *node = searchParaminGlobalList(param->value.s);
        if(node == NULL || node->paramSubscriptionStatus == OFF)
        {
			att.name = strdup(param->value.s);
			att.value = strdup("1");
			att.type = WDMP_INT;
			setAttributes(&att, 1, NULL, &wret);
			if (wret == WDMP_SUCCESS)
			{
				if (node == NULL) 
				{
					WalInfo("parameter: %s is not found in the globallist. Adding.\n", att.name);
					addParamToGlobalList(att.name, DYNAMIC_PARAM, ON);
					if (!writeDynamicParamToDBFile(att.name))
					{
						WalError("Write to DB file failed for '%s'\n", att.name);
					}
				}
				else
				{
					updateParamStatus(node, ON);
				}
				paramStatus[i] = WDMP_SUCCESS;
			}
			else
			{
				failure = true;
				paramStatus[i] = WDMP_FAILURE;
				WalError("setAttributes failed for '%s' ret:%d\n", param->value.s,wret);
			}
			WAL_FREE(att.name);
            WAL_FREE(att.value);
		}
		else
		{
			WalInfo("Parameter %s is already subscribed. Skipping setAttributes.\n",param->value.s);
			paramStatus[i] = WDMP_SUCCESS;
		}
    }

    // Prepare response
	if(failure == true && methodReq->objectCnt > 1)
	{
		cJSON *response = cJSON_CreateObject();
		cJSON *success_parameters = cJSON_CreateArray();
		cJSON *failure_parameters = cJSON_CreateArray();
		for (size_t i = 0; i < methodReq->objectCnt; i++)
		{
			const char *paramName =
            (methodReq->objects[i].params &&
             methodReq->objects[i].params[0].value.s)
            ? methodReq->objects[i].params[0].value.s : "Unknown";

			if(paramStatus[i] == WDMP_SUCCESS)
			{
				cJSON_AddItemToArray(success_parameters, cJSON_CreateString(paramName));
			}
			else
			{
				cJSON *resParamObj = cJSON_CreateObject();
				cJSON_AddItemToArray(failure_parameters, resParamObj);
				cJSON_AddStringToObject(resParamObj, "name", paramName);
				cJSON_AddStringToObject(resParamObj, "reason", "Failed to turn notification ON");
			}

		}
		cJSON_AddStringToObject(response, "message", "Partial success");
		cJSON_AddItemToObject(response, "success", success_parameters);
		cJSON_AddItemToObject(response, "failure",failure_parameters );
		resObj->u.methodRes->statusCode = WDMP_ERR_MULTI_STATUS;
		resObj->u.methodRes->message = cJSON_PrintUnformatted(response);
		cJSON_Delete(response);
	}
	else if(failure == true)
	{
		resObj->u.methodRes->statusCode = WDMP_ERR_NOTIF_ON_FAILED;
		resObj->u.methodRes->message = strdup("Failed to turn notification ON");
	}
	else
	{
		resObj->u.methodRes->statusCode = WDMP_SUCCESS;
		resObj->u.methodRes->message = strdup("Success");
	}
	
	WAL_FREE(paramStatus);
    WalInfo("validate_method_req: Processed '%s' method\n", methodReq->methodName);
    return;	
}