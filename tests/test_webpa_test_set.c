/**
 *  Copyright 2010-2013 Comcast Cable Communications Management, LLC
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
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <rbus/rbus.h>
#include <string.h>
#include "../source/broadband/include/webpa_internal.h"
#include "../source/include/webpa_adapter.h"
#include <cimplog/cimplog.h>
#include <wdmp-c.h>
#include <cJSON.h>
#include "ccsp_dm_api.h"

#define UNUSED(x) (void )(x)
#define MAX_PARAMETER_LEN			512

int getWebpaParameterValues(char **parameterNames, int paramCount, int *val_size, parameterValStruct_t ***val)
{
    UNUSED(parameterNames); UNUSED(paramCount); UNUSED(val_size); UNUSED(val);
    return (int) mock();
}

int setWebpaParameterValues(parameterValStruct_t *val, int paramCount, char **faultParam )
{
    UNUSED(faultParam); UNUSED(paramCount); UNUSED(val);
    return (int) mock();
}

void test_processRequest_test_and_set()
{
    char *reqPayload = "{\"command\":\"TEST_AND_SET\",\"old-cid\":\"61f4db9\",\"new-cid\":\"0\",\"sync-cmc\":\"512\",\"parameters\":[{\"name\":\"Device.X_RDK_WebConfig.URL\",\"dataType\":0,\"value\":\"https://cpe-config.xdp.comcast.net/api/v1/device/{mac}/config\",\"attributes\":{\"notify\":1}}]}";
    char *transactionId = "aasfsdfgehhdysy";
    char *resPayload = NULL;    
    headers_t *res_headers = NULL;
    headers_t *req_headers = NULL;
    extern int cachingStatus;

    getCompDetails();

    parameterValStruct_t **cmcList1 = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cmcList1[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cmcList1[0]->parameterName = strndup(PARAM_CMC,MAX_PARAMETER_LEN);
    cmcList1[0]->parameterValue = strndup("512",MAX_PARAMETER_LEN);
    cmcList1[0]->type = ccsp_int;
    will_return(get_global_values, cmcList1);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    parameterValStruct_t **cidList = (parameterValStruct_t **) malloc(sizeof(parameterValStruct_t*));
    cidList[0] = (parameterValStruct_t *) malloc(sizeof(parameterValStruct_t)*1);
    cidList[0]->parameterName = strndup(PARAM_CID,MAX_PARAMETER_LEN);
    cidList[0]->parameterValue = strndup("0",MAX_PARAMETER_LEN);
    cidList[0]->type = ccsp_string;
    will_return(get_global_values, cidList);
    will_return(get_global_parameters_count, 1);
    expect_function_call(CcspBaseIf_getParameterValues);
    will_return(CcspBaseIf_getParameterValues, CCSP_SUCCESS);
    expect_value(CcspBaseIf_getParameterValues, size, 1);

    processRequest(reqPayload, transactionId, &resPayload, req_headers, res_headers);
    WalInfo("resPayload : %s\n",resPayload);    
    if(resPayload !=NULL)
    {
	    free(resPayload);
    }
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_processRequest_test_and_set),         
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}