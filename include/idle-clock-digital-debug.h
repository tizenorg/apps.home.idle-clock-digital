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

#ifndef __IDLE_CLOCK_DIGITAL_DEBUG_H__
#define __IDLE_CLOCK_DIGITAL_DEBUG_H__

#include <unistd.h>
#include <dlog.h>

#define COLOR_GREEN 	"\033[0;32m"
#define COLOR_END		"\033[0;m"

#undef LOG_TAG
#define LOG_TAG "IDLE-CLOCK-DIGITAL"
#define _E(fmt, arg...) LOGE("[%s:%d] "fmt,__FUNCTION__,__LINE__,##arg)
#define _D(fmt, arg...) LOGD("[%s:%d] "fmt,__FUNCTION__,__LINE__,##arg)
#define _SECURE_E(fmt, arg...) SECURE_LOGE("[%s:%d] "fmt,__FUNCTION__,__LINE__,##arg)
#define _SECURE_D(fmt, arg...) SECURE_LOGD("[%s:%d] "fmt,__FUNCTION__,__LINE__,##arg)
#define ENTER() LOGD(COLOR_GREEN"BEGIN >>>"COLOR_END);
#define retvm_if_timer(timer, expr, val, fmt, arg...) do { \
	if(expr) { \
		_E(fmt, ##arg); \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		timer = NULL; \
		return (val); \
	} \
} while (0)

#define retvm_if(expr, val, fmt, arg...) do { \
	if(expr) { \
		_E(fmt, ##arg); \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define retv_if(expr, val) do { \
	if(expr) { \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define retm_if(expr, fmt, arg...) do { \
	if(expr) { \
		_E(fmt, ##arg); \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)

#define ret_if(expr) do { \
	if(expr) { \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)


//assert
#define ASSERT_SE(expr, args...) do { \
	if(expr) app_assert_screen(LOG_TAG, __FILE__, __LINE__, __func__, ##args); \
	} while (0)

#define ASSERT_S(expr) do { \
	if(expr) assert_screen(LOG_TAG, __FILE__, __LINE__, __func__, #expr, NULL); \
	} while (0)

void assert_screen(const char* tag_name, const char* file, int line, const char* func, const char *expr, const char *fmt, ...);

#endif /* __IDLE_CLOCK_DIGITAL_DEBUG_H__ */
