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


#ifndef __IDLE_CLOCK_DIGITAL_APP_DATA_H__
#define __IDLE_CLOCK_DIGITAL_APP_DATA_H__

#include <Elementary.h>
#include <appcore-efl.h>
#include <Ecore_X.h>
#include <app.h>

/* for time variables */
#include <unicode/utypes.h>
#include <unicode/putil.h>
#include <unicode/uiter.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>

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
	service_h service;

	/* for time display */
	Eina_Bool is_pre;
	int timeformat;
	char *timeregion_format;
	char *timezone_id;
	UDateFormat *formatter_time;
	UDateFormat *formatter_ampm;
	UDateFormat *formatter_time_24;
	UDateFormat *formatter_date;
	UDateTimePatternGenerator *generator;

	int clock_font;
	int clock_font_color;

	Eina_Bool is_show;
} appdata;

#endif /* __IDLE_CLOCK_DIGITAL_APP_DATA_H__ */

