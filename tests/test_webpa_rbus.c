#include<stdio.h>
#include <CUnit/Basic.h>

#include <cJSON.h>
#include <trower-base64/base64.h>
#include "../source/include/webpa_rbus.h"
rbusHandle_t handle;

/* Stubs for the parodus/notification symbols referenced by the asynchronous
 * OPERATE path. test_webpa_rbus links only webpa_rbus.c, so these are provided
 * locally; they are not exercised while the bus is uninitialized. */
char deviceMAC[32] = "112233445566";
void getDeviceMac()
{
}

void sendNotification(char *payload, char *source, char *destination)
{
    WalInfo("stub sendNotification dest=%s\n", (destination != NULL) ? destination : "NULL");
    if(payload != NULL)
    {
        free(payload);
    }
    if(source != NULL)
    {
        free(source);
    }
}


// Test case for isRbusEnabled
void test_isRbusEnabled_success()
{
    WalInfo("\n**************************************************\n");
	bool result = isRbusEnabled();
	CU_ASSERT_TRUE(result);
}

// Test case for isRbusInitialized success
void test_isRbusInitialized_success()
{
    WalInfo("\n**************************************************\n");
	webpaRbusInit("componentName");
	bool result = isRbusInitialized();
        printf("The bool value is %d\n", result);

	CU_ASSERT_TRUE(result);
	webpaRbus_Uninit();
}

void test_webpaRbusInit_success()
{
    WalInfo("\n**************************************************\n");
    int result = webpaRbusInit("component");
    CU_ASSERT_EQUAL(result, 0);
	webpaRbus_Uninit();
}

//Successcase for setTraceContext
void test_setTraceContext_success()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    //allocating moemory for each element
    traceContext[0] = strdup("randomvalueone");
    traceContext[1] = strdup("randomvaluetwo");
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);
    free(traceContext[0]);
    free(traceContext[1]);
}

//traceContext header NULL
void test_setTraceContext_header_NULL()
{   
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];
    traceContext[0] = NULL;
    traceContext[1] = NULL;
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
}

//traceContext header empty
void test_setTraceContext_header_empty()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];
    traceContext[0] = "";
    traceContext[1] = "";
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
}

void test_getTraceContext_success()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    //allocating moemory for each element
    traceContext[0] = strdup("randomvalueone");
    traceContext[1] = strdup("randomvaluetwo");
    rc = setTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);

    getTraceContext(traceContext);
    CU_ASSERT_EQUAL(0,rc);
    clearTraceContext();
}

void test_getTraceContext_empty()
{
    WalInfo("\n**************************************************\n");
    rbusError_t rc = RBUS_ERROR_BUS_ERROR;
    char* traceContext[2];

    getTraceContext(traceContext);
    CU_ASSERT_EQUAL(1,rc);
}

/*----------------------------------------------------------------------------*/
/*                        WebPA OPERATE (method invoke) tests                 */
/*----------------------------------------------------------------------------*/

/* buildOperateInParams flattens params.parameters into a flat rbusObject_t so
 * that providers reading rbusObject_GetValue(inParams, "<key>") work unchanged. */
void test_buildOperateInParams_flattens_keys()
{
    WalInfo("\n**************************************************\n");
    const char *json = "{\"linux_interface_name\":{\"value\":\"erouter0\",\"dataType\":0},"
                       "\"IPv4_DNS_Servers\":{\"value\":\"8.8.8.8\",\"dataType\":0}}";
    cJSON *parameters = cJSON_Parse(json);
    rbusObject_t inParams = NULL;
    WDMP_STATUS status = buildOperateInParams(parameters, &inParams);

    CU_ASSERT_EQUAL(status, WDMP_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(inParams);
    if(inParams != NULL)
    {
        rbusValue_t v1 = rbusObject_GetValue(inParams, "linux_interface_name");
        rbusValue_t v2 = rbusObject_GetValue(inParams, "IPv4_DNS_Servers");
        CU_ASSERT_PTR_NOT_NULL(v1);
        CU_ASSERT_PTR_NOT_NULL(v2);
        if(v1 != NULL)
        {
            int len = 0;
            const char *s = rbusValue_GetString(v1, &len);
            CU_ASSERT_STRING_EQUAL(s, "erouter0");
        }
        rbusObject_Release(inParams);
    }
    cJSON_Delete(parameters);
}

/* An absent/empty params object yields a valid, empty inParams object. */
void test_buildOperateInParams_empty()
{
    WalInfo("\n**************************************************\n");
    rbusObject_t inParams = NULL;
    WDMP_STATUS status = buildOperateInParams(NULL, &inParams);

    CU_ASSERT_EQUAL(status, WDMP_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(inParams);
    if(inParams != NULL)
    {
        CU_ASSERT_PTR_NULL(rbusObject_GetValue(inParams, "anything"));
        rbusObject_Release(inParams);
    }
}

/* Invalid base64 in the RDK.Operate value is rejected without invoking RBUS and
 * yields a base64 error envelope (no method name resolved). */
void test_webpaRbusOperate_invalid_base64()
{
    WalInfo("\n**************************************************\n");
    char *method = (char *) 0x1; /* ensure it is cleared to NULL */
    char *result = (char *) 0x1; /* ensure it is set to an error envelope */
    WDMP_STATUS status = webpaRbusOperate("!!!not-base64!!!", &method, &result);

    CU_ASSERT_NOT_EQUAL(status, WDMP_SUCCESS);
    CU_ASSERT_PTR_NULL(method);
    CU_ASSERT_PTR_NOT_NULL(result);
    if(result != NULL)
    {
        free(result);
    }
}

/* A decoded payload lacking a 'method' field is rejected without invoking RBUS;
 * an error envelope is produced and no method name is resolved. */
void test_webpaRbusOperate_missing_method()
{
    WalInfo("\n**************************************************\n");
    /* base64 of: {"params":{"parameters":{}}} */
    const char *jsonNoMethod = "{\"params\":{\"parameters\":{}}}";
    size_t encLen = 0;
    char *encoded = b64_encode_with_alloc((const uint8_t *) jsonNoMethod, strlen(jsonNoMethod), &encLen);
    char *method = NULL;
    char *result = NULL;
    WDMP_STATUS status;

    CU_ASSERT_PTR_NOT_NULL(encoded);
    status = webpaRbusOperate(encoded, &method, &result);
    CU_ASSERT_NOT_EQUAL(status, WDMP_SUCCESS);
    CU_ASSERT_PTR_NULL(method);
    CU_ASSERT_PTR_NOT_NULL(result);
    if(result != NULL)
    {
        free(result);
    }
    if(encoded != NULL)
    {
        free(encoded);
    }
}

/* A well-formed payload whose method cannot be invoked (bus not initialized)
 * maps to a WDMP failure status, echoes the method name, and returns an error
 * envelope. */
void test_webpaRbusOperate_invoke_failure_maps_to_failure()
{
    WalInfo("\n**************************************************\n");
    const char *validJson = "{\"method\":\"Device.NoSuchMethod()\",\"params\":{\"parameters\":{}}}";
    size_t encLen = 0;
    char *encoded = b64_encode_with_alloc((const uint8_t *) validJson, strlen(validJson), &encLen);
    char *method = NULL;
    char *result = NULL;
    WDMP_STATUS status;

    CU_ASSERT_PTR_NOT_NULL(encoded);
    webpaRbus_Uninit();
    status = webpaRbusOperate(encoded, &method, &result);
    CU_ASSERT_NOT_EQUAL(status, WDMP_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(method);
    if(method != NULL)
    {
        CU_ASSERT_STRING_EQUAL(method, "Device.NoSuchMethod()");
        free(method);
    }
    CU_ASSERT_PTR_NOT_NULL(result);
    if(result != NULL)
    {
        free(result);
    }
    if(encoded != NULL)
    {
        free(encoded);
    }
}

/* A payload carrying rspDestination selects the asynchronous path. With the bus
 * uninitialized the async invoke cannot proceed, so it returns a failure status
 * with a base64 error envelope while still echoing the method name. */
void test_webpaRbusOperate_async_rspDestination()
{
    WalInfo("\n**************************************************\n");
    const char *asyncJson = "{\"method\":\"Device.WiFi.CsiData.StartStream()\","
                            "\"params\":{\"parameters\":{}},"
                            "\"rspDestination\":\"event:some-dest/thing\"}";
    size_t encLen = 0;
    char *encoded = b64_encode_with_alloc((const uint8_t *) asyncJson, strlen(asyncJson), &encLen);
    char *method = NULL;
    char *result = NULL;
    WDMP_STATUS status;

    CU_ASSERT_PTR_NOT_NULL(encoded);
    webpaRbus_Uninit();
    status = webpaRbusOperate(encoded, &method, &result);
    CU_ASSERT_NOT_EQUAL(status, WDMP_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(method);
    if(method != NULL)
    {
        CU_ASSERT_STRING_EQUAL(method, "Device.WiFi.CsiData.StartStream()");
        free(method);
    }
    CU_ASSERT_PTR_NOT_NULL(result);
    if(result != NULL)
    {
        free(result);
    }
    if(encoded != NULL)
    {
        free(encoded);
    }
}

void add_suites( CU_pSuite *suite )
{
	*suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "test isRbusEnabled_success", test_isRbusEnabled_success);
    CU_add_test( *suite, "test isRbusInitialized_success", test_isRbusInitialized_success);
    CU_add_test( *suite, "test webpaRbusInit_success", test_webpaRbusInit_success);
    CU_add_test( *suite, "test setTraceContext_success", test_setTraceContext_success);
    CU_add_test( *suite, "test setTraceContext_header_NULL", test_setTraceContext_header_NULL);
    CU_add_test( *suite, "test setTraceContext_header_empty", test_setTraceContext_header_empty);
    CU_add_test( *suite, "test getTraceContext_success", test_getTraceContext_success);
    CU_add_test( *suite, "test getTraceContext_empty", test_getTraceContext_empty);
    CU_add_test( *suite, "test buildOperateInParams_flattens_keys", test_buildOperateInParams_flattens_keys);
    CU_add_test( *suite, "test buildOperateInParams_empty", test_buildOperateInParams_empty);
    CU_add_test( *suite, "test webpaRbusOperate_invalid_base64", test_webpaRbusOperate_invalid_base64);
    CU_add_test( *suite, "test webpaRbusOperate_missing_method", test_webpaRbusOperate_missing_method);
    CU_add_test( *suite, "test webpaRbusOperate_invoke_failure_maps_to_failure", test_webpaRbusOperate_invoke_failure_maps_to_failure);
    CU_add_test( *suite, "test webpaRbusOperate_async_rspDestination", test_webpaRbusOperate_async_rspDestination);
}


int main( int argc, char *argv[] )
{
	unsigned rv = 1;
    CU_pSuite suite = NULL;
 
    (void ) argc;
    (void ) argv;
    
    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }
        CU_cleanup_registry();
    }
    return rv;
}


