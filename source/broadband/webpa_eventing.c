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

int validateDynamicEventingData(method_req_t *methodReq)
{
    for (size_t i = 0; i < methodReq->objectCnt; i++)
    {
        method_param_t *obj = &methodReq->objects[i];
		param_t *param = &obj->params[0];
		    // Ensure we have at least 2 parameters
		if (obj->paramCnt < 2) 
		{
			WalError("Insufficient parameters\n");
			return WDMP_ERR_NOTIF_FIELD_COUNT;
		}
		// Validate 'name' key
		if(strcmp(param->name, "name") != 0)
		{
			WalError("Missing name field in method request\n");
			return WDMP_ERR_NOTIF_NAME_FIELD;
		}
		// Validate 'name' key
		if(param->value == NULL)
		{
			WalError("Missing name field in method request\n");
			return WDMP_ERR_NOTIF_NAME_MISSING;
		}

		param = &obj->params[1];
		// Validate 'notificationType' key
		if(strcmp(param->name, "notificationType") != 0)
		{
			WalError("Missing notificationType field in method request\n");
			return WDMP_ERR_NOTIF_TYPE_FIELD;
		}
		// Validate 'notificationType' value
		if(param->value == 0)
		{
			WalError("Missing param notificationType field in method reques\n");
			return WDMP_ERR_NOTIF_TYPE_MISSING;
		}		
		// Validate 'notificationType' value
		if(strcmp(param->value, "ValueChange") != 0)
		{
			WalError("Notification type is not supported:%s\n",param->value);
			return WDMP_ERR_NOTIF_TYPE_INVALID;
		}
	}
	return WDMP_SUCCESS;
}

void ProcessNotifyParamMethod(method_req_t *methodReq, res_struct *resObj)
{
    if (methodReq == NULL)
    {
        WalError("ProcessNotifyParamMethod: methodReq is NULL\n");
        return;
    }
    param_t att;
	WDMP_STATUS wret = WDMP_FAILURE;
	memset(&att, 0, sizeof(att));

	wret = validateDynamicEventingData(methodReq);
	if(wret != WDMP_SUCCESS)
	{
		resObj->retStatus[0] = wret;
		WalError("validateDynamicEventingData failed for method '%s' \n", methodReq->methodName);
		return;
	}

    // Process each parameter object
    for (size_t i = 0; i < methodReq->objectCnt; i++)
    {
        method_param_t *obj = &methodReq->objects[i];
		param_t *param = &obj->params[0];
		resObj->u.paramRes->params[i].name = strdup(param->value);
		g_NotifyParam *node = searchParaminGlobalList(param->value);
        if(node == NULL || node->paramSubscriptionStatus == OFF)
        {
			att.name = strdup(param->value);
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
				resObj->retStatus[i] = wret;
			} 
			else 
			{
				resObj->retStatus[i] = WDMP_ERR_NOTIF_ON_FAILED;
				WalError("setAttributes failed for '%s' ret:%d\n", param->value,wret);
			}
			WAL_FREE(att.name);
            WAL_FREE(att.value);
		}
		else
		{
			WalInfo("Parameter %s is already subscribed. Skipping setAttributes.\n",param->value);
			resObj->retStatus[i] = WDMP_SUCCESS;			
		}
    }

    WalInfo("validate_method_req: Processed '%s' method\n", methodReq->methodName);
    return;	
}