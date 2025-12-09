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
#include "webpa_adapter.h"

#ifdef BUILD_YOCTO
#define NOTIFY_PARAM_FILE "/nvram/webpa_notify_param"
#else
#define NOTIFY_PARAM_FILE "/tmp/webpa_notify_param"
#endif

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

void readDynamicParamsFromDBFile(int *notifyListSize);
int writeDynamicParamToDBFile(const char *param);
g_NotifyParam* searchParaminGlobalList(const char *paramName);
void addParamToGlobalList(const char* paramName, bool paramType, bool paramSubscriptionStatus);
char* CreateJsonFromGlobalNotifyList();
void setBootupNotifyInitDone(bool value);
bool getBootupNotifyInitDone();
g_NotifyParam* getGlobalNotifyHead();
bool getParamStatus(g_NotifyParam *param);
void updateParamStatus(g_NotifyParam *param, bool status);
int validate_params(method_param_t *obj);
void ProcessNotifyParamMethod(method_req_t *methodReq, res_struct *resObj);
#endif // WEBPA_EVENTING_H