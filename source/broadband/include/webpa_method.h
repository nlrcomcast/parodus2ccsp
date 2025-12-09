/**
 * @file webpa_method.h
 *
 * @description This file describes the Webpa Abstraction Layer
 *
 * Copyright (c) 2025  Comcast
 */
#include "webpa_adapter.h"
#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>s
int validate_method_req(method_req_t *methodReq);
rbusHandle_t get_webpa_rbus_Handle(void);