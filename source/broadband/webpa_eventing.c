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
#include "include/webpa_eventing.h"
#include "webpa_adapter.h"

g_NotifyParam *g_NotifyParamHead = NULL;
g_NotifyParam *g_NotifyParamTail = NULL;
pthread_mutex_t g_NotifyParamMut = PTHREAD_MUTEX_INITIALIZER;

bool initialNotifyInProgress = true; //By default set to true until initial notify ON is done minimum one iteration

bool getInitialNotifyInProgress()
{
    return initialNotifyInProgress;
}

void setInitialNotifyInProgress(bool value)
{
   initialNotifyInProgress  = value;
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
    g_NotifyParam *copy = NULL;

    pthread_mutex_lock(&g_NotifyParamMut);
	g_NotifyParam *temp = g_NotifyParamHead;
	while(temp != NULL)
	{
		if(strcmp(temp->paramName,paramName) == 0)
		{
            copy = (g_NotifyParam *)malloc(sizeof(g_NotifyParam));
            if (copy)
            {
                memcpy(copy, temp, sizeof(g_NotifyParam));
                copy->paramName = strdup(temp->paramName);
                copy->next = NULL;
            }
            else
            {
                WalError("Memory allocation failed in searchParaminGlobalList\n");
            }
            break;
		}
		temp = temp->next;
	}
    pthread_mutex_unlock(&g_NotifyParamMut);

	return copy;
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
		fprintf(fp,"%s,",param);
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

char* readDynamicParamsFromDBFile()
{
	FILE *fp;
	long file_size = 0;
	size_t read_size = 0;
	char *paramList = NULL;
	
	WalInfo("Dynamic parameters reading from DB %s\n",NOTIFY_PARAM_FILE);

	if (access(NOTIFY_PARAM_FILE, F_OK) != 0)
	{
		WalInfo("No dynamic parameters were available for this device\n");
		return NULL;
	}

	fp = fopen(NOTIFY_PARAM_FILE , "r");
	if (fp == NULL)
	{
		WalError("Failed to open file in db '%s' for read\n", NOTIFY_PARAM_FILE);
		return NULL;
	}

    // Move to end to determine file size
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);  // Go back to beginning

	if(file_size <= 0)
	{
		WalError("Dynamic parameter list is empty\n");
        fclose(fp);
		return NULL;
	}

    // Allocate memory to hold the entire file content
    paramList = (char *)malloc(file_size + 1);
    if (paramList == NULL)
	{
        WalError("Memory allocation failed while reading DB file %s\n", NOTIFY_PARAM_FILE);
        fclose(fp);
        return NULL;
    }

    // Read entire file into paramList
    read_size = fread(paramList, 1, file_size, fp);
    paramList[read_size] = '\0';
	fclose(fp);

	WalInfo("Successfully read %zu bytes from %s\n", read_size, NOTIFY_PARAM_FILE);
	return paramList;
}

char* CreateJsonFromGlobalNotifyList()
{
	char *paramList = NULL;
    bool status = 0;
    WalInfo("Inside CreateJsonFromGlobalNotifyList function\n");
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
