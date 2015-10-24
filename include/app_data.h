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

#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <Elementary.h>
#include <appcore-efl.h>
#include <Ecore_X.h>
#include <app.h>

/* for time variables */
#include <utils_i18n.h>

typedef struct __appdata
{
	Evas_Object *win;
	Evas_Object *ly_main;
	Evas_Object *bg;

	int win_w;
	int win_h;

	Ecore_Timer *timer;
	int win_type;
	Evas *e_offscreen;
	app_control_h app_control;

	/* for time display */
	Eina_Bool is_pre;
	int timeformat;
	char *timeregion_format;
	char *timezone_id;
	i18n_udate_format_h formatter_time;
	i18n_udate_format_h formatter_ampm;
	i18n_udate_format_h formatter_time_24;
	i18n_udate_format_h formatter_date;
	i18n_udatepg_h generator;

	Eina_Bool is_show;
} appdata;

#endif /* __APP_DATA_H__ */

