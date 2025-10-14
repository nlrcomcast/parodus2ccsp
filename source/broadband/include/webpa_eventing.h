/**
 * @file webpa_eventing.h
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2025  Comcast
 */
#ifndef WEBPA_EVENTING_H
#define WEBPA_EVENTING_H

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>

#define NOTIFY_PARAM_FILE "/nvram/webpa_notify_param"

#define DYNAMIC_PARAM 0
#define STATIC_PARAM 1
#define OFF 0
#define ON 1

typedef struct g_NotifyParam
{
    char *paramName;
    bool paramType;
    bool paramSubscriptionStatus;
    struct g_NotifyParam *next;
} g_NotifyParam;

char* readDynamicParamsFromDBFile();
int writeDynamicParamToDBFile(const char *param);
g_NotifyParam* searchParaminGlobalList(const char *paramName);
void addParamToGlobalList(const char* paramName, bool paramType, bool paramSubscriptionStatus);
char* CreateJsonFromGlobalNotifyList();
void setInitialNotifyInProgress(bool value);
bool getInitialNotifyInProgress();
g_NotifyParam* getGlobalNotifyHead();

#endif // WEBPA_EVENTING_H