#include "webpa_method.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <rbus/rbus.h>
#define UNUSED(x) (void )(x)
static rbusHandle_t rbus_handle;
rbusHandle_t get_webpa_rbus_Handle(void)
{
        int ret = RBUS_ERROR_SUCCESS;
        const char *pComponentName = WEBPA_COMPONENT_NAME;

        WalInfo("rbus_open for component %s\n", pComponentName);
        ret = rbus_open(&rbus_handle, pComponentName);
        if(ret != RBUS_ERROR_SUCCESS)
        {
                WalError("webpaRbusInit failed with error code %d\n", ret);
                return NULL;
        }
        WalInfo("webpaRbusInit is success. ret is %d\n", ret);
        return rbus_handle;
}

WDMP_STATUS mock_setAttributesReturn = WDMP_SUCCESS;
void setAttributes(param_t *att, const unsigned int count, money_trace_spans *transactionId, WDMP_STATUS *ret)
{
    UNUSED(transactionId);
    UNUSED(count);
    //UNUSED(att);
    WalInfo("mock_setAttributesReturn : %d\n",mock_setAttributesReturn);
    if(strstr(att->name, "ABC") != NULL)
    {
        *ret  = WDMP_FAILURE;
    }
    else
    {
        *ret = mock_setAttributesReturn;
    }
}

rbusError_t getTraceContext(char* traceContext[])
{
    UNUSED(traceContext);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t setTraceContext(char* traceContext[])
{
    UNUSED(traceContext);
    return RBUS_ERROR_SUCCESS;
}

void setValues(const param_t paramVal[], const unsigned int paramCount, const int setType, char *transactionId, money_trace_spans *timeSpan, WDMP_STATUS *retStatus, int *ccspStatus)
{
    UNUSED(paramVal); UNUSED(paramCount); UNUSED(setType); UNUSED(transactionId); UNUSED(timeSpan); UNUSED(retStatus); UNUSED(ccspStatus);
}

void addRowTable(char *objectName, TableData *list,char **retObject, WDMP_STATUS *retStatus)
{
    UNUSED(objectName); UNUSED(list); UNUSED(retObject); UNUSED(retStatus);
}
void deleteRowTable(char *object,WDMP_STATUS *retStatus)
{
    UNUSED(object); UNUSED(retStatus);
}

void replaceTable(char *objectName,TableData * list,unsigned int paramcount,WDMP_STATUS *retStatus)
{
    UNUSED(objectName); UNUSED(list); UNUSED(paramcount); UNUSED(retStatus);
}

char * getParameterValue(char *paramName)
{
    UNUSED(paramName);
    return NULL;
}

WDMP_STATUS setParameterValue(char *paramName, char* value, DATA_TYPE type)
{
    UNUSED(paramName); UNUSED(value); UNUSED(type);
    return WDMP_SUCCESS;
}

void getValues(const char *paramName[], const unsigned int paramCount, int index, money_trace_spans *timeSpan, param_t ***paramArr, int *retValCount, WDMP_STATUS *retStatus)
{
    UNUSED(paramName); UNUSED(paramCount); UNUSED(index); UNUSED(timeSpan); UNUSED(paramArr); UNUSED(retValCount); UNUSED(retStatus);
}

void getAttributes(const char *paramName[], const unsigned int paramCount, money_trace_spans *timeSpan, param_t **attr, int *retAttrCount, WDMP_STATUS *retStatus)
{
    UNUSED(paramName); UNUSED(paramCount); UNUSED(timeSpan); UNUSED(attr); UNUSED(retAttrCount); UNUSED(retStatus);
}

void test_processRequest_method_BootupInProgress(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.1.SSID\",\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    setBootupNotifyInitDone(true);
    WAL_FREE(resPayload);    
}


void test_processRequest_method(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.1.SSID\",\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_method_invalidName(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": 1234,\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_method_unsupported(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_ERR_UNSUPPORTED_NAMESPACE;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.2.SSID\",\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_method_type_unsupported(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.2.SSID\",\"notificationType\":\"ValueChange1\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_method_name_missed(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name1\": \"Test.WiFi.SSID.2.SSID\",\"notificationType\":\"ValueChange1\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_method_type_missed(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.2.SSID\",\"notificationType1\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_method_method_missed(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method1\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name1\": \"Test.WiFi.SSID.2.SSID\",\"notificationType\":\"ValueChange1\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);
}

void test_processRequest_method_parameters_missed(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters1\" : [{\"name\": \"Test.WiFi.SSID.2.SSID\",\"notificationType1\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_method_install_container(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Methods.SimpleMethod()\" ,\"parameters\" : [{\"URL\" : \"http://10.0.0.212/packages/cujo_agent.tar\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);
    rbus_close(rbus_handle);
}

void test_processRequest_method_empty_array(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Methods.SimpleMethod()\" ,\"parameters\" : [],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);   
}

void test_processRequest_method_no_parameters_key(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Methods.SimpleMethod()\" ,\"invalid_key\" : [{\"name\": \"Test.WiFi.SSID.1.SSID\",\"notificationType\":\"ValueChange\"},{\"name\": \"Test.WiFi.SSID.2.SSID\",\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);   
}

void test_processRequest_2params_sucess(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.1.SSID\",\"notificationType\":\"ValueChange\"},{\"name\": \"Test.WiFi.SSID.2.SSID\",\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_2params__partial_sucess(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.1.SSID\",\"notificationType\":\"ValueChange\"},{\"name\": \"ABC\",\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_2params__partial_sucess1(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name1\": \"Test.WiFi.SSID.1.SSID\",\"notificationType\":\"ValueChange\"},{\"name\": \"ABC\",\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_2params__partial_sucess2(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.1.SSID\",\"notificationType1\":\"ValueChange\"},{\"name\": \"ABC\",\"notificationType\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);    
}

void test_processRequest_2params__partial_sucess3(void)
{
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    mock_setAttributesReturn = WDMP_SUCCESS;
    char *reqPayload = "{ \"method\" : \"Device.Webpa.Subscription.NotifyEvent()\" ,\"parameters\" : [{\"name\": \"Test.WiFi.SSID.1.SSID\",\"notificationType\":\"ValueChange\"},{\"name\": \"ABC\",\"notificationType1\":\"ValueChange\"}],\"command\":\"METHOD\"}";
    char *resPayload = NULL;
    processRequest(reqPayload, NULL, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);
    WAL_FREE(resPayload);
}
/* =======================
   Test Suite Registration
   ======================= */

int main()
{
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("ProcessNotifyParamMethod Suite", NULL, NULL);
    CU_add_test(suite, "processRequest_method_BootupInProgress\n", test_processRequest_method_BootupInProgress);
    CU_add_test(suite, "processRequest_method\n", test_processRequest_method);
    CU_add_test(suite, "processRequest_method_param_unsupported\n", test_processRequest_method_unsupported);
    CU_add_test(suite, "processRequest_method_type_unsupported\n", test_processRequest_method_type_unsupported);
    CU_add_test(suite, "processRequest_method_name_missed\n", test_processRequest_method_name_missed);
    CU_add_test(suite, "processRequest_method_type_missed\n", test_processRequest_method_type_missed);
    CU_add_test(suite, "processRequest_2params_sucess\n", test_processRequest_2params_sucess);
    CU_add_test(suite, "processRequest_2params_partial_sucess\n", test_processRequest_2params__partial_sucess);
    CU_add_test(suite, "processRequest_2params_partial_sucess1\n", test_processRequest_2params__partial_sucess1);
    CU_add_test(suite, "processRequest_2params_partial_sucess2\n", test_processRequest_2params__partial_sucess2);
    CU_add_test(suite, "processRequest_2params_partial_sucess3\n", test_processRequest_2params__partial_sucess3);
    CU_add_test(suite, "processRequest_method_method_missed\n", test_processRequest_method_method_missed);
    CU_add_test(suite, "processRequest_method_parameters_missed\n", test_processRequest_method_parameters_missed);    
    CU_add_test(suite, "processRequest_method_install_container\n", test_processRequest_method_install_container); 
    CU_add_test(suite, "processRequest_method_empty_array\n", test_processRequest_method_empty_array);
    CU_add_test(suite, "processRequest_method_no_parameters_key\n", test_processRequest_method_no_parameters_key);
    CU_add_test(suite, "processRequest_method_invalidName\n", test_processRequest_method_invalidName);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return 0;
}
