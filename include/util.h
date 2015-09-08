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

#ifndef __UTIL_H__
#define __UTIL_H__

#include <Elementary.h>
#include <appcore-efl.h>
#include <Ecore_X.h>

#if !defined(PACKAGE)
#  define PACKAGE "idle-clock-digital"
#endif

#if !defined(RESDIR)
#  define RESDIR "/usr/apps/org.tizen.idle-clock-digital/res"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR RESDIR"/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR RESDIR"/edje"
#endif

#if !defined(PKGNAME)
#  define PKGNAME "org.tizen.idle-clock-digital"
#endif

#define EDJ_APP EDJDIR"/idle-clock-digital.edj"

#define S_(str) dgettext("sys_string", str)
#define T_(str) dgettext(PACKAGE, str)
// already defined in /usr/include/appcore/appcore-common.h:60:0
//#define N_(str) (str)
#undef _
#define _(str) gettext(str)

#define _EDJ(x) elm_layout_edje_get(x)
#define _X(x) (x*elm_config_scale_get())

#ifdef FEATURE_DIGITAL_OPERATOR_GEAR3
#define WIN_SIZE_W 360
#define WIN_SIZE_H 480
#else
#define WIN_SIZE_W 320
#define WIN_SIZE_H 320
#endif


typedef enum _buffer_type {
	BUFFER_TYPE_MINICONTROL = 0,
	BUFFER_TYPE_OFFSCREEN,
	BUFFER_TYPE_WINDOW,
} buffer_type_e;

#endif /* __IDLE_CLOCK_DIGITAL_COMMON_H__ */

