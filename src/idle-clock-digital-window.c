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


#include <Ecore.h>
#include <Ecore_X.h>
#include <minicontrol-provider.h>

#include "idle-clock-digital-app-data.h"
#include "idle-clock-digital-debug.h"
#include "idle-clock-digital-common.h"


Evas_Object *idle_clock_digital_window_create(const char *name)
{
	Evas_Object *eo = NULL;
	double scale = elm_config_scale_get();
	//eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	eo = minicontrol_win_add(name);

	if (eo) {
		//elm_win_alpha_set(eo, EINA_FALSE);
		elm_win_alpha_set(eo, EINA_TRUE);
		evas_object_resize(eo, WIN_SIZE*scale, WIN_SIZE*scale);
	}
	return eo;
}

