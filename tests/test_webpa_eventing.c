/**
 *  Copyright 2025-2026 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../source/include/webpa_adapter.h"
#include "../source/broadband/include/webpa_eventing.h"

// Externals from webpa_eventing.c
extern g_NotifyParam *g_NotifyParamHead;
extern g_NotifyParam *g_NotifyParamTail;
extern pthread_mutex_t g_NotifyParamMut;

#define UNUSED(x) (void )(x)
void setAttributes(param_t *attArr, const unsigned int paramCount, money_trace_spans *timeSpan, WDMP_STATUS *retStatus)
{
    UNUSED(attArr); UNUSED(paramCount); UNUSED(timeSpan); UNUSED(retStatus);

}

// === Helper to cleanup global list ===
static void cleanup_global_list(void)
{
    pthread_mutex_lock(&g_NotifyParamMut);
    g_NotifyParam *temp = g_NotifyParamHead;
    while (temp) {
        g_NotifyParam *next = temp->next;
        free(temp->paramName);
        free(temp);
        temp = next;
    }
    g_NotifyParamHead = g_NotifyParamTail = NULL;
    pthread_mutex_unlock(&g_NotifyParamMut);
}

// === TEST CASES ===

void test_add_and_search_param(void)
{
    cleanup_global_list();

    addParamToGlobalList("Device.DeviceInfo.SerialNumber", STATIC_PARAM, ON);
    addParamToGlobalList("Device.WiFi.SSID.1.SSID", DYNAMIC_PARAM, OFF);

    g_NotifyParam *p1 = searchParaminGlobalList("Device.DeviceInfo.SerialNumber");
    g_NotifyParam *p2 = searchParaminGlobalList("Device.WiFi.SSID.1.SSID");

    CU_ASSERT_PTR_NOT_NULL(p1);
    CU_ASSERT_PTR_NOT_NULL(p2);
    CU_ASSERT_STRING_EQUAL(p1->paramName, "Device.DeviceInfo.SerialNumber");
    CU_ASSERT_EQUAL(p2->paramType, DYNAMIC_PARAM);
}

void test_get_and_update_param_status(void)
{
    cleanup_global_list();

    addParamToGlobalList("Device.WiFi.Radio.1.Enable", STATIC_PARAM, OFF);
    g_NotifyParam *p = searchParaminGlobalList("Device.WiFi.Radio.1.Enable");
    CU_ASSERT_PTR_NOT_NULL(p);

    CU_ASSERT_FALSE(getParamStatus(p));
    updateParamStatus(p, ON);
    CU_ASSERT_TRUE(getParamStatus(p));
}

void test_write_and_read_file(void)
{
    cleanup_global_list();

    const char *test_file = "/tmp/test_webpa_notify_param";
    remove(test_file);

    int ret = writeDynamicParamToDBFile("Device.WiFi.SSID.1.SSID");
    CU_ASSERT_EQUAL(ret, 1);
    CU_ASSERT(access(NOTIFY_PARAM_FILE, F_OK) == 0);

    int count = 0;
    readDynamicParamsFromDBFile(&count);
    CU_ASSERT_EQUAL(count, 1);

    g_NotifyParam *p = searchParaminGlobalList("Device.WiFi.SSID.1.SSID");
    CU_ASSERT_PTR_NOT_NULL(p);
    CU_ASSERT_EQUAL(p->paramType, DYNAMIC_PARAM);
    CU_ASSERT_FALSE(p->paramSubscriptionStatus);

    remove(NOTIFY_PARAM_FILE);
}

void test_create_json_from_list(void)
{
    cleanup_global_list();

    addParamToGlobalList("Device.DeviceInfo.SerialNumber", STATIC_PARAM, ON);
    addParamToGlobalList("Device.WiFi.SSID.1.SSID", DYNAMIC_PARAM, OFF);

    char *json = CreateJsonFromGlobalNotifyList();
    CU_ASSERT_PTR_NOT_NULL(json);

    printf("Generated JSON: %s\n", json);
    CU_ASSERT_PTR_NOT_NULL(strstr(json, "Device.DeviceInfo.SerialNumber"));
    CU_ASSERT_PTR_NOT_NULL(strstr(json, "\"Status\":\"ON\""));
    CU_ASSERT_PTR_NOT_NULL(strstr(json, "\"Type\":\"Dynamic\"") || strstr(json, "\"Type\":\"Static\""));

    free(json);
}

void test_bootup_notify_flag(void)
{
    setBootupNotifyInitDone(false);
    CU_ASSERT_FALSE(getBootupNotifyInitDone());

    setBootupNotifyInitDone(true);
    CU_ASSERT_TRUE(getBootupNotifyInitDone());
}

// === MAIN ===
int main(void)
{
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    CU_pSuite suite = CU_add_suite("WebPA Eventing Tests", NULL, NULL);
    if (!suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_add_test(suite, "Add & Search Params", test_add_and_search_param);
    CU_add_test(suite, "Get & Update Param Status", test_get_and_update_param_status);
    CU_add_test(suite, "Write & Read File", test_write_and_read_file);
    CU_add_test(suite, "Create JSON from List", test_create_json_from_list);
    CU_add_test(suite, "Bootup Notify Flag", test_bootup_notify_flag);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
