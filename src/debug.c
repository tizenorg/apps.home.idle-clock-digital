/*
 *
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <app.h>
#include <unistd.h>
#include <errno.h>

#include "log.h"

void assert_screen(const char* tag_name, const char* file, int line, const char* func,  const char *expr, const char *fmt, ...)
{
    app_control_h app_control;
    va_list ap;
    char pid_buffer[16] = {0};
    char result_buffer[256] = {0};
    char line_buffer[16] = {0};

    app_control_create(&app_control);
    app_control_set_app_id(app_control, "com.samsung.assert-scr");
    snprintf(pid_buffer, sizeof(pid_buffer), "%d", getpid());
    app_control_add_extra_data(app_control, "pid", pid_buffer);
    app_control_add_extra_data(app_control, "appname", tag_name);
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
    app_control_add_extra_data(app_control, "assert_str", result_buffer);
    app_control_add_extra_data(app_control, "filename", file);
    snprintf(line_buffer, sizeof(line_buffer), "%d", line);
    app_control_add_extra_data(app_control, "line", line_buffer);
    app_control_add_extra_data(app_control, "funcname", func);
    app_control_send_launch_request(app_control, NULL,NULL );
    app_control_destroy(app_control);
}
