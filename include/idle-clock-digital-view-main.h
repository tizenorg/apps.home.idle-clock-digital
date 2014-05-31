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


#ifndef __IDLE_CLOCK_DIGITAL_VIEW_MAIN_H__
#define __IDLE_CLOCK_DIGITAL_VIEW_MAIN_H__

#include <Elementary.h>
#include <appcore-efl.h>
#include <Ecore_X.h>

bool idle_clock_digital_create_view_main(void *data);
int idle_clock_digital_parse_result_data(const char *result_data);
void idle_clock_digital_set_result_data(void *data);
void idle_clock_digital_update_view(void *data);
int _get_pm_state();
Eina_Bool __set_info_time(void *data);
void idle_clock_destroy_view_main(void *data);
void show_clock(void *data);
void hide_clock(void *data);

#endif /* __IDLE_CLOCK_DIGITAL_VIEW_MAIN_H__ */

