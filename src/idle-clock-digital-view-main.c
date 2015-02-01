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

#include <app.h>
#include <unistd.h>
#include <errno.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <glib.h>

#include <unicode/utypes.h>
#include <unicode/putil.h>
#include <unicode/uiter.h>
#include <unicode/udat.h>
#include <unicode/udatpg.h>
#include <unicode/ustring.h>

#include "idle-clock-digital-app-data.h"
#include "idle-clock-digital-view-main.h"
#include "idle-clock-digital-window.h"
#include "idle-clock-digital-debug.h"
#include "idle-clock-digital-common.h"

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlreader.h>
#include <string.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <E_DBus.h>

#define DEVICED_PROCESS_PATH		"/Org/Tizen/System/DeviceD/Display"
#define DEVICED_PROCESS_INTERFACE	"org.tizen.system.deviced.display"

#define SIGNAL_LCD_ON "LCDOn"
#define SIGNAL_LCD_OFF "LCDOff"

#define MAX_PATH_LENGTH 1024

typedef struct __setting_items {
	int show_date;
	int clock_font;
	int show_battery;
	int show_bluetooth;
	int clock_font_color;
} setting_items;

/*
clock_font: 1 - Default
clock_font: 2 - Light
clock_font: 3 - Dynamic


font_color : 1 - #000000
font_color : 2 - #CEFF00
font_color : 3 - #FF6519
font_color : 4 - #BCFFFB
font_color : 5 - #F03880
font_color : 6 - #FFEA00
font_color : 7 - #673E27
font_color : 8 - #FFFFFF
font_color : 9 - #042860
font_color : 10 - #F2DCC5
font_color : 11 - #F62E00
font_color : 12 - #595959

*/

static char font_color_list[][7] = { "000000", "CEFF00", "FF6519", "BCFFFB", "F03880", "FFEA00",
								"673E27", "FFFFFF", "042860", "F2DCC5", "F62E00", "595959"};

#define VCONF_PACKAGE_SETTING_FINISH	"memory/wms/clock_package_setting_finish"
#define DIGITAL_VCONF_SHOW_DATE			"db/idle-clock/digital/showdate"
#define DIGITAL_VCONF_CLOCK_FONT		"db/idle-clock/digital/clock_font"
#define DIGITAL_VCONF_CLOCK_FONT_COLOR	"db/idle-clock/digital/clock_font_color"

#define FONT_DEFAULT_FAMILY_NAME "SamsungSansNum3L"
#define FONT_DEFAULT_SIZE 96

#define FONT_LIGHT_FAMILY_NAME "GalaxyGear_2_1"
#define FONT_LIGHT_SIZE 116

#define FONT_DYNAMIC_FAMILY_NAME "GalaxyGear_3_2"
#define FONT_DYNAMIC_SIZE 90


#define FONTDIR "/usr/apps/org.tizen.idle-clock-digital/res/font"

setting_items Items = {0,};

#define BUFFER_LENGTH 256

static void __clock_font_changed_cb(keynode_t *node, void *data);

void show_clock(void *data)
{
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	elm_object_signal_emit(ad->ly_main, "show_effect", "");
	ad->is_show = true;
}

void hide_clock(void *data)
{
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	elm_object_signal_emit(ad->ly_main, "hide_effect", "");
	ad->is_show = false;
}

void idle_clock_digital_set_result_data(void *data)
{
	ENTER();
	appdata *ad = data;
	retm_if(ad == NULL, "appdata is NULL");

	int ret = 0;

	ret = vconf_set_int(DIGITAL_VCONF_SHOW_DATE, Items.show_date);
	retm_if(ret, "vconf_set_int(DIGITAL_VCONF_SHOW_DATE) failed");

	ret = vconf_set_int(DIGITAL_VCONF_CLOCK_FONT, Items.clock_font);
	retm_if(ret, "vconf_set_int(DIGITAL_VCONF_CLOCK_FONT) failed");

	ret = vconf_set_int(DIGITAL_VCONF_CLOCK_FONT_COLOR, Items.clock_font_color);
	retm_if(ret, "vconf_set_int(DIGITAL_VCONF_CLOCK_FONT_COLOR) failed");

	__clock_font_changed_cb(NULL, ad);
}

int idle_clock_digital_parse_result_data(const char *result_data)
{
	ENTER();
	retvm_if(!result_data, -1, "result is NULL");

	int i = 0;
	xmlDocPtr doc = NULL;
	xmlXPathContextPtr xpath_context = NULL;
	xmlXPathObjectPtr xpath_obj_organization = NULL;
	xmlChar *xpath_organization = (xmlChar*)"/Application/SettingsResult/Item";

	xmlInitParser();

	doc = xmlParseMemory(result_data, strlen(result_data));
	if (doc == NULL) {
	    _E("unable to xmlParseMemory");
		return -1;
	}

	/* Create xpath evaluation context */
	xpath_context = xmlXPathNewContext(doc);
	if (xpath_context == NULL) {
	    _E("unable to create new XPath context");
		xmlFreeDoc(doc);
		return -1;
	}

	xpath_obj_organization = xmlXPathEvalExpression(xpath_organization, xpath_context);
	if (xpath_obj_organization == NULL) {
		_E("unable to xmlXPathEvalExpression!");
		xmlXPathFreeContext(xpath_context);
		xmlFreeDoc(doc);
		return -1;
	}

	if (xpath_obj_organization->nodesetval->nodeNr) {
		_D("node count [%d]", xpath_obj_organization->nodesetval->nodeNr);
	}
	else {
		_E("xmlXPathEvalExpression failed");
		xmlXPathFreeObject(xpath_obj_organization);
		xmlXPathFreeContext(xpath_context);
		xmlFreeDoc(doc);
		return -1;
	}

	for(i = 0; i < xpath_obj_organization->nodesetval->nodeNr; i++) {
		xmlNodePtr itemNode = xpath_obj_organization->nodesetval->nodeTab[i];
		if(itemNode) {
			char *id = NULL;
			id = (char *) xmlGetProp(itemNode, (const xmlChar *)"id");
			if(!id) {
				_E("xmlGetProp failed");
				goto FINISH_OFF;
			}

			if(strcmp(id, "showdate") == 0) {
				xmlNodePtr childNode = xmlFirstElementChild(itemNode);
				char *checked = NULL;

				while(childNode) {
					checked = (char *) xmlGetProp(childNode, (const xmlChar *)"checked");
					if(checked) {
						_D("checked:%s", checked);
						if(strcmp(checked, "yes") == 0) {
							Items.show_date = 1;
						} else if(strcmp(checked, "no") == 0) {
							Items.show_date = 0;
						}
						xmlFree(checked);
					}
					childNode = xmlNextElementSibling(childNode);
				}
				xmlFree(childNode);
			} else if(strcmp(id, "clock_font") == 0) {
				xmlNodePtr childNode = xmlFirstElementChild(itemNode);
				char *selected = NULL;
				int num = 0;

				while(childNode) {
					selected = (char *) xmlGetProp(childNode, (const xmlChar *)"selected");
					if(selected) {
						_D("clock_font: selected:%s", selected);
						num = atoi(selected);
						Items.clock_font = num;
						xmlFree(selected);
					}
					childNode = xmlNextElementSibling(childNode);
				}
				xmlFree(childNode);
			} else if(strcmp(id, "clock_font_color") == 0) {
				xmlNodePtr childNode = xmlFirstElementChild(itemNode);
				char *selected = NULL;
				int num = 0;

				while(childNode) {
					selected = (char *) xmlGetProp(childNode, (const xmlChar *)"selected");
					if(selected) {
						_D("clock_font_color: selected:%s", selected);
						num = atoi(selected);
						Items.clock_font_color = num;
						xmlFree(selected);
					}
					childNode = xmlNextElementSibling(childNode);
				}
				xmlFree(childNode);
			}
			xmlFree(id);
		}
	}

FINISH_OFF:
	if(xpath_obj_organization)
		xmlXPathFreeObject(xpath_obj_organization);

	if(xpath_context)
		xmlXPathFreeContext(xpath_context);

	if(doc)
		xmlFreeDoc(doc);

	xmlCleanupParser();

	return 0;
}

static char *__get_locale(void)
{
	char *locale = vconf_get_str(VCONFKEY_REGIONFORMAT);
	if(locale == NULL) {
		_E("vconf_get_str() failed : region format");
		return strdup("en_US");
	}

	char *str = NULL;
	int count = 0;
	gchar **ptr = NULL;
	gchar **split_str = NULL;

	split_str = g_strsplit(locale, ".", 0);

	for(ptr = split_str; *ptr; ptr++) {
		count++;
	}

	_D("count:%d", count);
	_D("orig_locale:%s", locale);

	if(count == 2) {
		if(!strcmp(split_str[1], "UTF-8")) {
			str = g_strdup_printf("%s", split_str[0]);
			free(locale);
			_D("dest_locale:%s", str);
			g_strfreev(split_str);
			return str;
		} else {
			g_strfreev(split_str);
			return locale;
		}
	} else {
		g_strfreev(split_str);
		return locale;
	}
}

static UDateTimePatternGenerator *__get_generator(void *data)
{
	_D("");

	UErrorCode status = U_ZERO_ERROR;
	UDateTimePatternGenerator *generator = NULL;

	UChar u_skeleton[64] = {0,};

	appdata *ad = data;
	retv_if(ad == NULL, NULL);


	uloc_setDefault(getenv("LC_TIME"), &status);
	if (U_FAILURE(status)) {
		_E("uloc_setDefault() is failed.");
		return NULL;
	}

	u_uastrncpy(u_skeleton, "hhmm", strlen("hhmm"));

	if(ad->timeregion_format == NULL) {
		ad->timeregion_format = __get_locale();
	}

	generator = udatpg_open(ad->timeregion_format, &status);
	if(U_FAILURE(status)) {
		_E("udatpg_open() failed");
		generator = NULL;
		return NULL;
	}

	_D("get_generator success");
	return generator;
}

static UDateFormat *__get_time_formatter(void *data)
{
	_D("");

	UErrorCode status = U_ZERO_ERROR;

	UChar u_pattern[64] = {0,};
	UChar u_timezone[64] = {0,};
	UChar u_best_pattern[64] = {0,};
	int32_t u_best_pattern_capacity;
	UDateFormat *formatter = NULL;

	appdata *ad = data;
	retv_if(ad == NULL, NULL);

	if(ad->generator == NULL) {
		ad->generator = __get_generator(ad);
	}

	/* only 12 format */
	if(u_uastrncpy(u_pattern, "h:mm", sizeof(u_pattern)) == NULL) {
		_E("u_uastrncpy() is failed.");
		return NULL;
	}

	u_best_pattern_capacity =
			(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));

	udatpg_getBestPattern(ad->generator, u_pattern, u_strlen(u_pattern),
								u_best_pattern, u_best_pattern_capacity, &status);
	if(U_FAILURE(status)) {
		_E("udatpg_getBestPattern() failed");
		return NULL;
	}

	/* remove am/pm of best pattern */
	char a_best_pattern[64] = {0.};
	u_austrcpy(a_best_pattern, u_best_pattern);
	_D("best pattern [%s]", a_best_pattern);
	char *a_best_pattern_fixed = strtok(a_best_pattern, "a");
	a_best_pattern_fixed = strtok(a_best_pattern_fixed, " ");
	_D("best pattern fixed [%s]", a_best_pattern_fixed);

	if(a_best_pattern_fixed) {
		/* exception - da_DK */
		if(strncmp(ad->timeregion_format, "da_DK", 5) == 0
		|| strncmp(ad->timeregion_format, "mr_IN", 5) == 0){

			char *a_best_pattern_changed = g_strndup("h:mm", 4);
			_D("best pattern is changed [%s]", a_best_pattern_changed);
			if(a_best_pattern_changed) {
				u_uastrcpy(u_best_pattern, a_best_pattern_changed);
				g_free(a_best_pattern_changed);
			}
		}
		else {
			u_uastrcpy(u_best_pattern, a_best_pattern_fixed);
		}
	}

	/* change char to UChar */
	u_strncpy(u_pattern, u_best_pattern, sizeof(u_pattern));

	/* get formatter */
	u_uastrncpy(u_timezone, ad->timezone_id, sizeof(u_timezone));
	formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, ad->timeregion_format, u_timezone, -1,
							u_pattern, -1, &status);
	if (U_FAILURE(status)) {
		_E("udat_open() is failed.");
		return NULL;
	}

	_D("getting time formatter success");
	return formatter;
}

static UDateFormat *__get_time_formatter_24(void *data)
{
	_D("");

	UErrorCode status = U_ZERO_ERROR;

	UChar u_pattern[64] = {0,};
	UChar u_timezone[64] = {0,};
	UChar u_best_pattern[64] = {0,};
	int32_t u_best_pattern_capacity;
	UDateFormat *formatter = NULL;

	appdata *ad = data;
	retv_if(ad == NULL, NULL);

	if(ad->generator == NULL) {
		ad->generator = __get_generator(ad);
	}

	/* only 12 format */
	if(u_uastrncpy(u_pattern, "H:mm", sizeof(u_pattern)) == NULL) {
		_E("u_uastrncpy() is failed.");
		return NULL;
	}

	u_best_pattern_capacity =
			(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));

	udatpg_getBestPattern(ad->generator, u_pattern, u_strlen(u_pattern),
								u_best_pattern, u_best_pattern_capacity, &status);
	if(U_FAILURE(status)) {
		_E("udatpg_getBestPattern() failed");
		return NULL;
	}

	/* remove am/pm of best pattern */
	char a_best_pattern[64] = {0.};
	u_austrcpy(a_best_pattern, u_best_pattern);
	_D("best pattern [%s]", a_best_pattern);
	char *a_best_pattern_fixed = strtok(a_best_pattern, "a");
	a_best_pattern_fixed = strtok(a_best_pattern_fixed, " ");
	_D("best pattern fixed [%s]", a_best_pattern_fixed);

	if(a_best_pattern_fixed) {
		/* exception - pt_BR(HH'h'mm), id_ID, da_DK */
		if(strncmp(a_best_pattern_fixed, "HH'h'mm", 7) == 0
			|| strncmp(ad->timeregion_format, "id_ID", 5) == 0
			|| strncmp(ad->timeregion_format, "da_DK", 5) == 0
			|| strncmp(ad->timeregion_format, "mr_IN", 5) == 0){

			char *a_best_pattern_changed = g_strndup("HH:mm", 5);
			_D("best pattern is changed [%s]", a_best_pattern_changed);
			if(a_best_pattern_changed) {
				u_uastrcpy(u_best_pattern, a_best_pattern_changed);
				g_free(a_best_pattern_changed);
			}
		}
		else {
			u_uastrcpy(u_best_pattern, a_best_pattern_fixed);
		}
	}

	/* change char to UChar */
	u_strncpy(u_pattern, u_best_pattern, sizeof(u_pattern));

	/* get formatter */
	u_uastrncpy(u_timezone, ad->timezone_id, sizeof(u_timezone));
	formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, ad->timeregion_format, u_timezone, -1,
							u_pattern, -1, &status);
	if (U_FAILURE(status)) {
		_E("udat_open() is failed.");
		return NULL;
	}

	_D("getting time formatter success");
	return formatter;
}

static UDateFormat *__get_date_formatter(void *data)
{
	_D("");

	UErrorCode status = U_ZERO_ERROR;
	UChar u_timezone[64] = {0,};
	UChar u_skeleton[64] = {0,};
	int skeleton_len = 0;

	UChar u_best_pattern[64] = {0,};
	int32_t u_best_pattern_capacity;
	UDateFormat *formatter = NULL;

	appdata *ad = data;
	retv_if(ad == NULL, NULL);

	u_uastrncpy(u_skeleton, "MMMEd", strlen("MMMEd"));
	skeleton_len = u_strlen(u_skeleton);

	u_best_pattern_capacity =
					(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));
	udatpg_getBestPattern(ad->generator, u_skeleton, skeleton_len,
							u_best_pattern, u_best_pattern_capacity, &status);
	if(U_FAILURE(status)) {
		_E("udatpg_getBestPattern() failed");
		return NULL;
	}

	if(strncmp(ad->timeregion_format, "fi_FI", 5) == 0) {
		char *a_best_pattern_changed = g_strndup("ccc, d. MMM", 11);
		_D("date formatter best pattern is changed [%s]", a_best_pattern_changed);
		if(a_best_pattern_changed) {
			u_uastrcpy(u_best_pattern, a_best_pattern_changed);
			g_free(a_best_pattern_changed);
		}
	}

	/* get formatter */
	u_uastrncpy(u_timezone, ad->timezone_id, sizeof(u_timezone));
	formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, ad->timeregion_format, u_timezone, -1,
							u_best_pattern, -1, &status);
	if (U_FAILURE(status)) {
		_E("udat_open() is failed.");
		return NULL;
	}

	_D("getting date formatter success");

	return formatter;

}


static UDateFormat *__get_ampm_formatter(void *data)
{
	_D("");

	UErrorCode status = U_ZERO_ERROR;

	char a_best_pattern[64] = {0.};

	UChar u_timezone[64] = {0,};
	UChar u_skeleton[64] = {0,};
	int skeleton_len = 0;

	UChar u_best_pattern[64] = {0,};
	int32_t u_best_pattern_capacity;
	UDateFormat *formatter = NULL;

	appdata *ad = data;
	retv_if(ad == NULL, NULL);

	u_uastrncpy(u_skeleton, "hhmm", strlen("hhmm"));
	skeleton_len = u_strlen(u_skeleton);

	u_best_pattern_capacity =
					(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));
	udatpg_getBestPattern(ad->generator, u_skeleton, skeleton_len,
							u_best_pattern, u_best_pattern_capacity, &status);
	if(U_FAILURE(status)) {
		_E("udatpg_getBestPattern() failed");
		return NULL;
	}

	u_austrcpy(a_best_pattern, u_best_pattern);
	u_uastrcpy(u_best_pattern, "a");

	if(a_best_pattern[0] == 'a') {
		ad->is_pre = EINA_TRUE;
	} else {
		ad->is_pre = EINA_FALSE;
	}

	/* get formatter */
	u_uastrncpy(u_timezone, ad->timezone_id, sizeof(u_timezone));
	formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, ad->timeregion_format, u_timezone, -1,
							u_best_pattern, -1, &status);
	if (U_FAILURE(status)) {
		_E("udat_open() is failed.");
		return NULL;
	}

	_D("getting ampm formatter success");

	return formatter;

}

static void __set_formatters(void *data)
{
	_D();

	appdata *ad = data;
	ret_if(ad == NULL);

	/* generator */
	ad->generator = __get_generator(ad);
	/* time formatter */
	ad->formatter_time = __get_time_formatter(ad);
	/* ampm formatter */
	ad->formatter_ampm = __get_ampm_formatter(ad);
	/* 24 time formatter */
	ad->formatter_time_24 = __get_time_formatter_24(ad);
	/* date formatter */
	ad->formatter_date = __get_date_formatter(ad);
}

static void __remove_formatters(void *data)
{
	_D();

	appdata *ad = data;
	ret_if(ad == NULL);

	if(ad->generator) {
		udat_close(ad->generator);
		ad->generator = NULL;
	}
	if(ad->formatter_time) {
		udat_close(ad->formatter_time);
		ad->formatter_time = NULL;
	}
	if(ad->formatter_ampm) {
		udat_close(ad->formatter_ampm);
		ad->formatter_ampm = NULL;
	}
	if(ad->formatter_time_24) {
		udat_close(ad->formatter_time_24);
		ad->formatter_time_24 = NULL;
	}
	if(ad->formatter_date) {
		udat_close(ad->formatter_date);
		ad->formatter_date = NULL;
	}
}

static int __get_formatted_date_from_utc_time(void *data, time_t intime, char *buf, int buf_len)
{
	appdata *ad = data;
	retv_if(ad == NULL, -1);
	retv_if(ad->formatter_date == NULL, -1);

	UDate u_time = (UDate)intime * 1000;
	UChar u_formatted_str[64] = {0,};
	int32_t u_formatted_str_capacity;
	int formatted_str_len = -1;
	UErrorCode status = U_ZERO_ERROR;

	/* calculate formatted string capacity */
	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	/* fomatting date using formatter */
	formatted_str_len = udat_format(ad->formatter_date, u_time, u_formatted_str, u_formatted_str_capacity,
							NULL, &status);
	if(U_FAILURE(status)) {
		_E("udat_format() failed");
		return -1;
	}

	if(formatted_str_len <= 0)
		_E("formatted_str_len is less than 0");

	buf = u_austrncpy(buf, u_formatted_str, buf_len);
	_SECURE_D("date:(%d)[%s][%d]", formatted_str_len, buf, intime);

	return 0;
}

static int __get_formatted_ampm_from_utc_time(void *data, time_t intime, char *buf, int buf_len, int *ampm_len)
{
	appdata *ad = data;
	retv_if(ad == NULL, -1);
	retv_if(ad->formatter_ampm == NULL, -1);

	UDate u_time = (UDate)intime * 1000;
	UChar u_formatted_str[64] = {0,};
	int32_t u_formatted_str_capacity;
	int formatted_str_len = -1;
	UErrorCode status = U_ZERO_ERROR;

	/* calculate formatted string capacity */
	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	/* fomatting date using formatter */
	formatted_str_len = udat_format(ad->formatter_ampm, u_time, u_formatted_str, u_formatted_str_capacity,
									NULL, &status);
	if(U_FAILURE(status)) {
		_E("udat_format() failed");
		return -1;
	}

	if(formatted_str_len <= 0)
		_E("formatted_str_len is less than 0");

	(*ampm_len) = u_strlen(u_formatted_str);

	buf = u_austrncpy(buf, u_formatted_str, buf_len);
	_SECURE_D("ampm:(%d)[%s][%d]", formatted_str_len, buf, intime);

	return 0;
}

static int __get_formatted_time_from_utc_time(void *data, time_t intime, char *buf, int buf_len, Eina_Bool is_time_24)
{
	appdata *ad = data;
	retv_if(ad == NULL, -1);

	UDate u_time = (UDate)intime * 1000;
	UChar u_formatted_str[64] = {0,};
	int32_t u_formatted_str_capacity;
	int formatted_str_len = -1;
	UErrorCode status = U_ZERO_ERROR;

	/* calculate formatted string capacity */
	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	/* fomatting date using formatter */
	if(is_time_24) {
		retv_if(ad->formatter_time_24 == NULL, -1);
		formatted_str_len = udat_format(ad->formatter_time_24, u_time, u_formatted_str, u_formatted_str_capacity,
								NULL, &status);
	} else {
		retv_if(ad->formatter_time == NULL, -1);
		formatted_str_len = udat_format(ad->formatter_time, u_time, u_formatted_str, u_formatted_str_capacity,
								NULL, &status);
	}
	if(U_FAILURE(status)) {
		_E("udat_format() failed");
		return -1;
	}

	if(formatted_str_len <= 0)
		_E("formatted_str_len is less than 0");

	buf = u_austrncpy(buf, u_formatted_str, buf_len);
	_SECURE_D("time:(%d)[%s][%d]", formatted_str_len, buf, intime);

	return 0;
}

static char *replaceAll(char *s, const char *olds, const char *news)
{
  char *result, *sr;
  size_t i, count = 0;
  size_t oldlen = strlen(olds); if (oldlen < 1) return s;
  size_t newlen = strlen(news);


  if (newlen != oldlen) {
    for (i = 0; s[i] != '\0';) {
      if (memcmp(&s[i], olds, oldlen) == 0) count++, i += oldlen;
      else i++;
    }
  } else i = strlen(s);


  result = (char *) malloc(i + 1 + count * (newlen - oldlen));
  if (result == NULL) return NULL;


  sr = result;
  while (*s) {
    if (memcmp(s, olds, oldlen) == 0) {
      memcpy(sr, news, newlen);
      sr += newlen;
      s  += oldlen;
    } else *sr++ = *s++;
  }
  *sr = '\0';

  return result;
}

Eina_Bool __set_info_time(void *data)
{
	appdata *ad = data;
	if (ad == NULL) {
		_D("appdata is NULL");
		return ECORE_CALLBACK_RENEW;
	}

	struct tm *ts = NULL;
	time_t tt;
	int err_code = 0;
	int showdate = 0;
	int clock_font = ad->clock_font;
	int clock_font_color = 0;
#ifdef EMULATOR_TYPE
	char font_buf[512] = {0, };
#endif
	tt = time(NULL);
	ts = localtime(&tt);

	if (ad->timer != NULL) {
		ecore_timer_del(ad->timer);
		ad->timer = NULL;
	}

	int val;
	vconf_get_int(VCONFKEY_PM_STATE, &val);
	if(val != VCONFKEY_PM_STATE_LCDOFF) {
		ad->timer = ecore_timer_add(60 - ts->tm_sec, __set_info_time, ad);
	}

	char utc_date[256] = { 0, }; //text_date

	/* text_date */
	err_code = vconf_get_int(DIGITAL_VCONF_CLOCK_FONT_COLOR, &clock_font_color);
	retvm_if(err_code, ECORE_CALLBACK_RENEW, "vconf_get_int(DIGITAL_VCONF_CLOCK_FONT_COLOR) failed");

	if(clock_font_color < 1 || clock_font_color > 12) //Exception
		clock_font_color = 8;

	err_code = vconf_get_int(DIGITAL_VCONF_SHOW_DATE, &showdate);
	retvm_if(err_code, ECORE_CALLBACK_RENEW, "vconf_get_int(DIGITAL_VCONF_SHOW_DATE) failed");

	_D("show_date:%d", showdate);

#ifdef EMULATOR_TYPE
	__get_formatted_date_from_utc_time(ad, tt, utc_date, sizeof(utc_date));
	elm_object_part_text_set(ad->ly_main, "default_text_date", utc_date);
	snprintf(font_buf, sizeof(font_buf)-1, "%s_%d", "show,default_text_date", clock_font_color);
	elm_object_signal_emit(ad->ly_main, font_buf, "source_default_text_date");
#else
	if(showdate) {
		__get_formatted_date_from_utc_time(ad, tt, utc_date, sizeof(utc_date));
		elm_object_part_text_set(ad->ly_main, "default_text_date", utc_date);
	}
	_D();
#endif

	char utc_time[BUFFER_LENGTH] = { 0 };
	char utc_ampm[BUFFER_LENGTH] = { 0 };
	int ampm_length = 0;
	Eina_Bool is_pre = ad->is_pre;
	Eina_Bool is_24hour = EINA_FALSE;

	if(ad->timeformat == VCONFKEY_TIME_FORMAT_12) {
		__get_formatted_ampm_from_utc_time(ad, tt, utc_ampm, sizeof(utc_ampm), &ampm_length);
		__get_formatted_time_from_utc_time(ad, tt, utc_time, sizeof(utc_time), EINA_FALSE);
		is_24hour = EINA_FALSE;
	} else {
		__get_formatted_time_from_utc_time(ad, tt, utc_time, sizeof(utc_time), EINA_TRUE);
		is_24hour = EINA_TRUE;
	}

	_D("utc_time=%s, utc_ampm=[%d]%s", utc_time, ampm_length, utc_ampm);

	if(ampm_length >= 3) {
		_D("AM PM string is too long, changed to default AM/PM");
		if (ts->tm_hour >= 0 && ts->tm_hour < 12)
			snprintf(utc_ampm, sizeof(utc_ampm), "%s", "AM");
		else
			snprintf(utc_ampm, sizeof(utc_ampm), "%s", "PM");
	}

	char *time_str = NULL;

	/*to show arabic colon, replace normal colon to custom colond(u+2236)*/
	if(strncmp(ad->timeregion_format, "ar", 2) == 0) {
		char *arabic_text = NULL;
		if(clock_font == 3) {
			arabic_text = replaceAll(utc_time, ":", "∶");
			_D("arabic:%s", arabic_text);

			memset(utc_time, 0, sizeof(utc_time));
			snprintf(utc_time, sizeof(utc_time), "%s", arabic_text);
			free(arabic_text);
		}
	}

	if(is_24hour == EINA_TRUE){
		time_str = g_strdup_printf("<color=#%sFF>%s</color>", font_color_list[clock_font_color-1], utc_time);
	}
	else {
#ifdef EMULATOR_TYPE
		if(is_pre== EINA_TRUE)
			time_str = g_strdup_printf("<color=#%sFF><font_size=24>%s</font_size>%s</color>", font_color_list[clock_font_color-1], utc_ampm, utc_time);
		else {
			// Todo : Fix the gap between clock and ampm.
			if(clock_font == 1) {
				time_str = g_strdup_printf("<color=#%sFF>%s<font_size=24> %s</font_size></color>", font_color_list[clock_font_color-1], utc_time, utc_ampm);
			} else {
				time_str = g_strdup_printf("<color=#%sFF>%s<font_size=24>  %s</font_size></color>", font_color_list[clock_font_color-1], utc_time, utc_ampm);
			}
		}
#else
		if(is_pre== EINA_TRUE)
			time_str = g_strdup_printf("<color=#%sFF><font_size=24><font=SamsungSans:style=Regular>%s</font></font_size>%s</color>", font_color_list[clock_font_color-1], utc_ampm, utc_time);
		else {
			// Todo : Fix the gap between clock and ampm.
			if(clock_font == 1) {
				time_str = g_strdup_printf("<color=#%sFF>%s<font_size=24><font=SamsungSans:style=Regular> %s</font></font_size></color>", font_color_list[clock_font_color-1], utc_time, utc_ampm);
			} else {
				time_str = g_strdup_printf("<color=#%sFF>%s<font_size=24><font=SamsungSans:style=Regular>  %s</font></font_size></color>", font_color_list[clock_font_color-1], utc_time, utc_ampm);
			}
		}
#endif
	}
	_D("time_str=%s", time_str);

	elm_object_part_text_set(ad->ly_main, "textblock_time", time_str);

	switch(clock_font) {
		case 1:
			elm_object_signal_emit(ad->ly_main, "change,default", "source_textblock_time");
			break;
		case 2:
			elm_object_signal_emit(ad->ly_main, "change,light", "source_textblock_time");
			break;
		case 3:
			elm_object_signal_emit(ad->ly_main, "change,dynamic", "source_textblock_time");
			break;
		default:
			elm_object_signal_emit(ad->ly_main, "change,default", "source_textblock_time");
			break;
	}

	g_free(time_str);

	return ECORE_CALLBACK_RENEW;
}

void idle_clock_digital_update_view(void *data)
{
	appdata *ad = data;
	if (ad == NULL) {
		_E("appdata is NULL");
		return;
	}

	if(ad->win != NULL) {
		__set_info_time(ad);
	}
}

static UChar *uastrcpy(const char *chars)
{
	int len = 0;
	UChar *str = NULL;
	len = strlen(chars);
	str = (UChar *) malloc(sizeof(UChar) *(len + 1));
	if (!str)
		return NULL;
	u_uastrcpy(str, chars);
	return str;
}

static void ICU_set_timezone(const char *timezone)
{
	_D("%s", __func__);
	if(timezone == NULL) {
		_E("TIMEZONE is NULL");
		return;
	}

//	DBG("ICU_set_timezone = %s ", timezone);
	UErrorCode ec = U_ZERO_ERROR;
	UChar *str = uastrcpy(timezone);

	ucal_setDefaultTimeZone(str, &ec);
	if (U_SUCCESS(ec)) {
//		DBG("ucal_setDefaultTimeZone() SUCCESS ");
	} else {
		_E("ucal_setDefaultTimeZone() FAILED : %s ",
			      u_errorName(ec));
	}
	free(str);
}

static char* __get_timezone()
{
	char buf[1024];
	ssize_t len = readlink("/opt/etc/localtime", buf, sizeof(buf)-1);

	if (len != -1) {
		buf[len] = '\0';
	}
	else {
		/* handle error condition */
		_E("readlink() failed");
		return vconf_get_str(VCONFKEY_SETAPPL_TIMEZONE_ID);
	}
	return strdup(buf+20);
}


static void __time_status_vconf_changed_cb(keynode_t *node, void *data)
{
	_D();
	appdata *ad = data;
	ret_if(ad == NULL);

	if(ad->timezone_id) {
		free(ad->timezone_id);
		ad->timezone_id = NULL;
	}
	if(ad->timeregion_format) {
		free(ad->timeregion_format);
		ad->timeregion_format = NULL;
	}

	ad->timezone_id = __get_timezone();
	ICU_set_timezone(ad->timezone_id);
	ad->timeregion_format = __get_locale();
	vconf_get_int(VCONFKEY_REGIONFORMAT_TIME1224, &ad->timeformat);

	_D("[%d][%s][%s]", ad->timeformat, ad->timeregion_format, ad->timezone_id);

	__remove_formatters(ad);
	__set_formatters(ad);

	if(node != NULL) {
		__set_info_time(ad);
	}
}

static void __clear_time(void *data)
{
	_D();
	appdata *ad = data;
	ret_if(ad == NULL);

	hide_clock(ad);

//	elm_object_part_text_set(ad->ly_main, "textblock_time", "");
//	elm_object_part_text_set(ad->ly_main, "text_date", "");
	if (ad->timer != NULL) {
		ecore_timer_del(ad->timer);
		ad->timer = NULL;
	}
}

static void __clock_font_changed_cb(keynode_t *node, void *data)
{
	appdata *ad = data;
	ret_if(ad == NULL);

	vconf_get_int(DIGITAL_VCONF_CLOCK_FONT, &ad->clock_font);
	int clock_font = ad->clock_font;

	/* font */
#ifdef EMULATOR_TYPE
	elm_config_font_overlay_set("idle_font", "", FONT_DEFAULT_SIZE);
#else
	switch(ad->clock_font) {
		case 1:
			_D("Default");
			elm_config_font_overlay_set("idle_font", FONT_DEFAULT_FAMILY_NAME, FONT_DEFAULT_SIZE);
			break;
		case 2:
			_D("Light");
			elm_config_font_overlay_set("idle_font", FONT_LIGHT_FAMILY_NAME, FONT_LIGHT_SIZE);
			break;
		case 3:
			_D("Dynamic");
			elm_config_font_overlay_set("idle_font", FONT_DYNAMIC_FAMILY_NAME, FONT_DYNAMIC_SIZE);
			break;
		default:
			elm_config_font_overlay_set("idle_font", FONT_DEFAULT_FAMILY_NAME, FONT_DEFAULT_SIZE);
			break;
	}
#endif

	elm_config_font_overlay_apply();

	int showdate = 0;
	vconf_get_int(DIGITAL_VCONF_SHOW_DATE, &showdate);
	int clock_font_color = 0;
	vconf_get_int(DIGITAL_VCONF_CLOCK_FONT_COLOR, &clock_font_color);
	char font_buf[512] = {0, };

	_D("show_date:%d, font:%d", showdate, clock_font);

	if(showdate) {
		if(clock_font == 1) {
			snprintf(font_buf, sizeof(font_buf)-1, "%s_%d", "show,default_text_date", clock_font_color);
			elm_object_signal_emit(ad->ly_main, font_buf, "source_default_text_date");
			elm_object_signal_emit(ad->ly_main, "change,default", "source_textblock_time");
		} else if(clock_font == 2) {
			snprintf(font_buf, sizeof(font_buf)-1, "%s_%d", "show,light_text_date", clock_font_color);
			elm_object_signal_emit(ad->ly_main, font_buf, "source_light_text_date");
			elm_object_signal_emit(ad->ly_main, "change,light", "source_textblock_time");
		} else if(clock_font == 3) {
			snprintf(font_buf, sizeof(font_buf)-1, "%s_%d", "show,dynamic_text_date", clock_font_color);
			elm_object_signal_emit(ad->ly_main, font_buf, "source_dynamic_text_date");
			elm_object_signal_emit(ad->ly_main, "change,dynamic", "source_textblock_time");
		}
	}
	else {
		elm_object_signal_emit(ad->ly_main, "hide,text_date", "source_text_date");
		elm_object_signal_emit(ad->ly_main, "change,no_data", "source_textblock_time");
	}

	if(node != NULL) {
		__set_info_time(ad);
	}
}

int _get_pm_state()
{
	int val;
	vconf_get_int(VCONFKEY_PM_STATE, &val);
	_D("PM STATE [%d]", val);
	return val;
}

static void __pm_changed_cb(keynode_t *node, void *data)
{
	appdata *ad = data;
	ret_if(ad == NULL);

	struct tm *ts = NULL;
	time_t tt;

	int val;
	vconf_get_int(VCONFKEY_PM_STATE, &val);
	_D("PM STATE [%d]", val);

	if(val == VCONFKEY_PM_STATE_NORMAL) {
		if(!ad->is_show) {
			__set_info_time(ad);
			show_clock(ad);
		}

		tt = time(NULL);
		ts = localtime(&tt);
		if (ad->timer) {
			ecore_timer_del(ad->timer);
			ad->timer = NULL;
		}
		ad->timer = ecore_timer_add(60 - ts->tm_sec, __set_info_time, ad);
	}
	else if(val == VCONFKEY_PM_STATE_LCDOFF) {
		//__clear_time(data);	//Disable this code for transit to alpm clock
	}
	else
		_D("Not interested PM STATE");
}

static void __language_changed_cb(keynode_t *node, void *data)
{
	_D("%s", __func__);
	appdata *ad = data;
	ret_if(ad == NULL);

	sleep(1);
	__time_status_vconf_changed_cb(0x1, ad);
}

void _setting_finish_changed_cb(keynode_t *node, void *data)
{
	_D("##### _setting_finish_changed_cb #####");
	appdata *ad = data;
	ret_if(ad == NULL);

	if(ad->win_type == BUFFER_TYPE_OFFSCREEN) {
		_E("##### I'm going to die!!!");
		elm_exit();
	}
}

static void __register_vconf(void *data)
{
	_D();
	appdata *ad = data;
	ret_if(ad == NULL);
	int ret = -1;

	/* register vconf changed cb */
	ret = vconf_notify_key_changed(VCONFKEY_SYSTEM_TIME_CHANGED, __time_status_vconf_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}
	/*Register changed cb(timezone)*/
	ret = vconf_notify_key_changed(VCONFKEY_SETAPPL_TIMEZONE_ID, __time_status_vconf_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}
	/* register vconf changed cb */
	ret = vconf_notify_key_changed(VCONFKEY_PM_STATE, __pm_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	/*Register changed cb(language)*/
	ret = vconf_notify_key_changed(VCONFKEY_LANGSET, __language_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	/*Register changed cb(regionformat)*/
	ret = vconf_notify_key_changed(VCONFKEY_REGIONFORMAT_TIME1224, __time_status_vconf_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	/*Register changed cb(preview finish)*/
	ret = vconf_notify_key_changed(VCONF_PACKAGE_SETTING_FINISH, _setting_finish_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

}

static void __deregister_vconf()
{
	int ret = -1;

	/* register vconf changed cb */
	ret = vconf_ignore_key_changed(VCONFKEY_SYSTEM_TIME_CHANGED, __time_status_vconf_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}
	/*Register changed cb(timezone)*/
	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_TIMEZONE_ID, __time_status_vconf_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}
	/* register vconf changed cb */
	ret = vconf_ignore_key_changed(VCONFKEY_PM_STATE, __pm_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	/*Register changed cb(language)*/
	ret = vconf_ignore_key_changed(VCONFKEY_LANGSET, __language_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	/*Register changed cb(regionformat)*/
	ret = vconf_ignore_key_changed(VCONFKEY_REGIONFORMAT_TIME1224, __time_status_vconf_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	/*Register changed cb(preview finish)*/
	ret = vconf_ignore_key_changed(VCONF_PACKAGE_SETTING_FINISH, _setting_finish_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	ret = vconf_ignore_key_changed(DIGITAL_VCONF_CLOCK_FONT, __clock_font_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	/* date showing */
	ret = vconf_ignore_key_changed(DIGITAL_VCONF_SHOW_DATE, __clock_font_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}

	/* clock_font_color */
	ret = vconf_ignore_key_changed(DIGITAL_VCONF_CLOCK_FONT_COLOR, __clock_font_changed_cb);
	if(ret < 0) {
		_E("Failed to register vconfkey changed cb.");
	}
}

static void __set_info(void *data)
{
	_D();
	appdata *ad = data;
	ret_if(ad == NULL);

	__register_vconf(data);

	/*Clock font*/
	vconf_get_int(DIGITAL_VCONF_CLOCK_FONT, &ad->clock_font);
	__time_status_vconf_changed_cb(NULL, ad);
	__clock_font_changed_cb(NULL, ad);

	__set_info_time(ad);
}

static void on_changed_receive(void *data, DBusMessage *msg)
{
	ENTER();
	appdata *ad = (appdata *)data;
	ret_if(ad == NULL);

	int lcd_on = 0;
	int lcd_off = 0;

	lcd_on = dbus_message_is_signal(msg, DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_ON);
	lcd_off = dbus_message_is_signal(msg, DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_OFF);

	if(lcd_on) {
		_D("%s - %s", DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_ON);
		__set_info_time(ad);
		show_clock(ad);
	} else if(lcd_off) {
		_D("%s - %s", DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_OFF);

	} else {
		_E("%s - %s", DEVICED_PROCESS_INTERFACE, "unknown signal");
	}
}


static void idle_clock_digital_register_lcd_onoff(void *data)
{
	E_DBus_Signal_Handler *handler = NULL;;
	E_DBus_Connection *conn = NULL;

	// Init
	ecore_init();
	e_dbus_init();

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		_E("e_dbus_bus_get error");
			return;
	}

	handler = e_dbus_signal_handler_add(conn, NULL, DEVICED_PROCESS_PATH,
										DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_ON,
										on_changed_receive, data);
	if (handler == NULL) {
			_E("e_dbus_signal_handler_add error");
			return;
	}
}

static void _ecore_evas_msg_parent_handle(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
	_D(" >>>>>>>>>>> Receive msg from clien msg_domain=%x msg_id=%x size=%d", msg_domain, msg_id, size);
	_D(" ########## data : %s", data);

	if(g_strcmp0(data, "mc_resume") == 0) {
		_D("##### mc_resume");
	} else if(g_strcmp0(data, "mc_pause") == 0) {
		_D("##### mc_pause");
	}
}

static Evas_Object *__add_layout(Evas_Object *parent, const char *file, const char *group)
{
	_D("%s", __func__);
	Evas_Object *eo = NULL;
	int r = -1;

	retvm_if(parent == NULL, NULL, "Invalid argument: parent is NULL\n");
	retvm_if(file == NULL, NULL, "Invalid argument: file is NULL\n");
	retvm_if(group == NULL, NULL, "Invalid argument: group is NULL\n");

	eo = elm_layout_add(parent);
	retvm_if(eo == NULL, NULL, "Failed to add layout\n");

	r = elm_layout_file_set(eo, file, group);
	if (!r) {
		_E("Failed to set file[%s]\n", file);
		evas_object_del(eo);
		return NULL;
	}

	evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(eo);
	return eo;
}

void idle_clock_destroy_view_main(void *data)
{
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	__remove_formatters(ad);
}

bool idle_clock_digital_create_view_main(void *data)
{
	appdata *ad = data;
	retvm_if(ad == NULL, false, "Invalid argument: appdata is NULL\n");

	/* create main layout */
	Evas_Object *ly_main = NULL;

	ly_main = __add_layout(ad->win, EDJ_APP, "layout_clock_digital");
	retvm_if(ly_main == NULL, -1, "Failed to add main layout\n");
	evas_object_size_hint_weight_set(ly_main, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ly_main);
	ad->ly_main = ly_main;
	evas_object_show(ad->win);

	Evas * evas = evas_object_evas_get(ad->win);
	Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas);
	ecore_evas_callback_msg_parent_handle_set(ee, _ecore_evas_msg_parent_handle);


	evas_object_resize(ly_main, WIN_SIZE_W, WIN_SIZE_H);
	evas_object_show(ly_main);

	idle_clock_digital_register_lcd_onoff(ad);

	__set_info(ad);

	return true;
}

