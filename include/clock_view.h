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

#ifndef __CLOCK_VIEW_H__
#define __CLOCK_VIEW_H__

#include <Elementary.h>
#include <appcore-efl.h>
#include <Ecore_X.h>



bool clock_view_create_layout(void *data);
int clock_view_parse_result_data(const char *result_data);
void clock_view_set_result_data(void *data);
void clock_view_update_view(void *data);
int clock_view_get_display_state();
Eina_Bool clock_view_set_info_time(void *data);
void clock_view_destroy_view_main(void *data);
void clock_view_show_clock(void *data);
void clock_view_hide_clock(void *data);



#endif /* __CLOCK_VIEW_H__ */
