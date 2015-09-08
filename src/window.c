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
#include <Ecore_X.h>
#include <minicontrol-provider.h>

#include "app_data.h"
#include "log.h"
#include "util.h"



Evas_Object *window_create(const char *name)
{
	Evas_Object *eo = NULL;
	double scale = elm_config_scale_get();
	//eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	eo = minicontrol_win_add(name);

	if (eo) {
		//elm_win_alpha_set(eo, EINA_FALSE);
		elm_win_alpha_set(eo, EINA_TRUE);
		evas_object_resize(eo, WIN_SIZE_W*scale, WIN_SIZE_H*scale);
	}
	return eo;
}

