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

#include <Ecore.h>
#include <Elementary.h>
#include <watch_app.h>
#include <watch_app_efl.h>


#include "app_data.h"
#include "log.h"
#include "util.h"



Evas_Object *window_create(const char *name)
{
	Evas_Object *win = NULL;
	int ret = 0;

	/* Window */

	_D("WATCH APP CREATE");
	ret = watch_app_get_elm_win(&win);
	if (ret != APP_ERROR_NONE) {
		_E("failed to get window(%d)", ret);
		return NULL;
	}

	elm_win_title_set(win, "idle-clock_digital");
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_alpha_set(win, EINA_FALSE);
	elm_win_indicator_opacity_set(win, ELM_WIN_INDICATOR_TRANSPARENT);
	elm_win_role_set(win, "no-effect");

	/* Show window after base gui is set up */
	evas_object_show(win);

	return win;

}

