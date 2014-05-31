/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <app.h>
#include <unistd.h>
#include <errno.h>

#include "idle-clock-digital-debug.h"

void assert_screen(const char* tag_name, const char* file, int line, const char* func,  const char *expr, const char *fmt, ...)
{
    service_h service;
    va_list ap;
    char pid_buffer[16] = {0};
    char result_buffer[256] = {0};
    char line_buffer[16] = {0};

    service_create(&service);
    service_set_package(service, "org.tizen.assert-scr");
    snprintf(pid_buffer, sizeof(pid_buffer), "%d", getpid());
    service_add_extra_data(service, "pid", pid_buffer);
    service_add_extra_data(service, "appname", tag_name);
    if(fmt == NULL)
    {
        snprintf(result_buffer, sizeof(result_buffer), "%s", expr);
    }
    else
    {
        char arg_buffer[256] = {0};

        va_start(ap, fmt);
        vsnprintf(arg_buffer, sizeof(arg_buffer), fmt, ap);
        va_end(ap);
        snprintf(result_buffer, sizeof(result_buffer), "(%s) %s", expr, arg_buffer);
    }
    service_add_extra_data(service, "assert_str", result_buffer);
    service_add_extra_data(service, "filename", file);
    snprintf(line_buffer, sizeof(line_buffer), "%d", line);
    service_add_extra_data(service, "line", line_buffer);
    service_add_extra_data(service, "funcname", func);
    service_send_launch_request(service, NULL,NULL );
    service_destroy(service);
}
