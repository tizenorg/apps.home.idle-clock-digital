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
#include <app_preference.h>
#include <unistd.h>
#include <device/display.h>
#include <device/callback.h>
#include <errno.h>
#include <glib.h>
#include <system_settings.h>

#include <utils_i18n.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlreader.h>
#include <string.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <E_DBus.h>

#include "app_data.h"
#include "clock_view.h"
#include "window.h"
#include "log.h"
#include "util.h"


#define DEVICED_PROCESS_PATH		"/Org/Tizen/System/DeviceD/Display"
#define DEVICED_PROCESS_INTERFACE	"org.tizen.system.deviced.display"

#define SIGNAL_LCD_ON "LCDOn"
#define SIGNAL_LCD_OFF "LCDOff"

#define MAX_PATH_LENGTH 1024

static struct {
	int show_date;
	int clock_font_color;
} setting_items_s =
{
	.show_date = 1,
	.clock_font_color = 8,
};

/*
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

#define DIGITAL_PREFERENCE_SHOW_DATE			"showdate"
#define DIGITAL_PREFERENCE_CLOCK_FONT_COLOR	"clock_font_color"

#define FONT_DEFAULT_FAMILY_NAME "Tizen:style=Bold"
#define FONT_DEFAULT_SIZE 80

#define FONTDIR "/usr/apps/org.tizen.idle-clock-digital/res/font"
#define BUFFER_LENGTH 256



static void _clock_font_changed_cb(int node, void *data);



void clock_view_show_clock(void *data)
{
	appdata *ad = data;
	ret_if(!ad);

	elm_object_signal_emit(ad->ly_main, "show_effect", "");
	ad->is_show = true;
}



void clock_view_hide_clock(void *data)
{
	appdata *ad = data;
	ret_if(!ad);

	elm_object_signal_emit(ad->ly_main, "hide_effect", "");
	ad->is_show = false;
}



void clock_view_set_result_data(void *data)
{
	appdata *ad = data;
	ret_if(!ad);

	int ret = 0;
	bool existing = false;

	ret = preference_is_existing(DIGITAL_PREFERENCE_SHOW_DATE, &existing);
	if (!ret) {
		if (!existing) ret = preference_set_int(DIGITAL_PREFERENCE_SHOW_DATE, setting_items_s.show_date);
		if (0 != ret) _E("cannot set the show date(%d)", ret);
	} else _E("cannot check the preference is existing(%d)");

	ret = preference_is_existing(DIGITAL_PREFERENCE_CLOCK_FONT_COLOR, &existing);
	if (!ret) {
		if (!existing) ret = preference_set_int(DIGITAL_PREFERENCE_CLOCK_FONT_COLOR, setting_items_s.clock_font_color);
		if (0 != ret) _E("cannot set the clock font color(%d)", ret);
	} else _E("cannot check the preference is existing(%d)");

	_clock_font_changed_cb(0, ad);
}



int clock_view_parse_result_data(const char *result_data)
{
	retv_if(!result_data, -1);

	int i = 0;
	xmlDocPtr doc = NULL;
	xmlXPathContextPtr xpath_context = NULL;
	xmlXPathObjectPtr xpath_obj_organization = NULL;
	xmlChar *xpath_organization = (xmlChar*)"/Application/SettingsResult/Item";

	xmlInitParser();

	doc = xmlParseMemory(result_data, strlen(result_data));
	if (!doc) {
	    _E("unable to xmlParseMemory");
		return -1;
	}

	/* Create xpath evaluation context */
	xpath_context = xmlXPathNewContext(doc);
	if (!xpath_context) {
	    _E("unable to create new XPath context");
		xmlFreeDoc(doc);
		return -1;
	}

	xpath_obj_organization = xmlXPathEvalExpression(xpath_organization, xpath_context);
	if (!xpath_obj_organization) {
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
							setting_items_s.show_date = 1;
						} else if(strcmp(checked, "no") == 0) {
							setting_items_s.show_date = 0;
						}
						xmlFree(checked);
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
						setting_items_s.clock_font_color = num;
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



static char *_get_locale(void)
{
	char *locale = NULL;

	int ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_COUNTRY, &locale);
	if (ret < 0) {
		_E("fail to get the locale country value(%d)", ret);
		return strdup("en_US");
	}

	if(!locale) {
		_E("system settings fail to get value: region format");
		return strdup("en_US");
	}
	_D("locale is %s", locale);

	return locale;
}



static i18n_udatepg_h _get_generator(void *data)
{
	_D("");

	int status = I18N_ERROR_INVALID_PARAMETER;
	i18n_udatepg_h generator = NULL;

	//UChar u_skeleton[64] = {0,};

	appdata *ad = data;
	retv_if(!ad, NULL);


	status = i18n_ulocale_set_default(NULL);
	if (status != I18N_ERROR_NONE) {
		_E("uloc_setDefault() is failed.");
		return NULL;
	}

	//i18n_ustring_copy_ua_n(u_skeleton, "hhmm", strlen("hhmm"));

	if(!ad->timeregion_format) {
		ad->timeregion_format = _get_locale();
	}

	status = i18n_udatepg_create(ad->timeregion_format, &generator);
	if(status != I18N_ERROR_NONE) {
		_E("udatepg_creation is failed");
		generator = NULL;
		return NULL;
	}

	_D("get_generator success");
	return generator;
}



static i18n_udate_format_h _get_time_formatter(void *data)
{
	_D("");

	char a_best_pattern[64] = {0.};
	char *a_best_pattern_fixed = NULL;


	int status = I18N_ERROR_INVALID_PARAMETER;

	i18n_uchar u_pattern[64] = {0,};
	i18n_uchar u_timezone[64] = {0,};
	i18n_uchar u_best_pattern[64] = {0,};
	int32_t u_best_pattern_capacity;
	int32_t best_pattern_len;
	i18n_udate_format_h formatter = NULL;

	appdata *ad = data;
	retv_if(!ad, NULL);

	if(!ad->generator) {
		ad->generator = _get_generator(ad);
	}

	/* only 12 format */

	if (!i18n_ustring_copy_ua_n(u_pattern, "h:mm", sizeof(u_pattern))) {
		_E("ustring_copy() is failed.");
		return NULL;
	}

	u_best_pattern_capacity =
			(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));

	status = i18n_udatepg_get_best_pattern(ad->generator, u_pattern, i18n_ustring_get_length(u_pattern),
								u_best_pattern, u_best_pattern_capacity, &best_pattern_len);
	if(status != I18N_ERROR_NONE) {
		_E("get best pattern() failed(%d)", status);
		return NULL;
	}

	/* remove am/pm of best pattern */
	retv_if(!i18n_ustring_copy_au(a_best_pattern, u_best_pattern), NULL);
	_D("best pattern [%s]", a_best_pattern);
	a_best_pattern_fixed = strtok(a_best_pattern, "a");
	a_best_pattern_fixed = strtok(a_best_pattern_fixed, " ");
	_D("best pattern fixed [%s]", a_best_pattern_fixed);

	if(a_best_pattern_fixed) {
		/* exception - da_DK */
		if(strncmp(ad->timeregion_format, "da_DK", 5) == 0
		|| strncmp(ad->timeregion_format, "mr_IN", 5) == 0){

			char *a_best_pattern_changed = g_strndup("h:mm", 4);
			_D("best pattern is changed [%s]", a_best_pattern_changed);
			if(a_best_pattern_changed) {
				i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_changed);
				g_free(a_best_pattern_changed);
			}
		}
		else {
			retv_if(!i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_fixed), NULL);
		}
	}

	/* change char to UChar */
	retv_if(!i18n_ustring_copy_n(u_pattern, u_best_pattern, sizeof(u_pattern)), NULL);

	/* get formatter */
	i18n_ustring_copy_ua_n(u_timezone, ad->timezone_id, sizeof(u_timezone));
	status = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, ad->timeregion_format, u_timezone, -1,
							u_pattern, -1, &formatter);
	if (!formatter) {
		_E("time_create() is failed.%d", status);

		return NULL;
	}

	_D("getting time formatter success");
	return formatter;
}



static i18n_udate_format_h _get_time_formatter_24(void *data)
{
	_D("");

	char a_best_pattern[64] = {0.};
	char *a_best_pattern_fixed = NULL;

	int status = I18N_ERROR_INVALID_PARAMETER;

	i18n_uchar u_pattern[64] = {0,};
	i18n_uchar u_timezone[64] = {0,};
	i18n_uchar u_best_pattern[64] = {0,};
	int32_t u_best_pattern_capacity;
	int32_t best_pattern_len;
	i18n_udate_format_h formatter = NULL;

	appdata *ad = data;
	retv_if(!ad, NULL);

	if(!ad->generator) {
		ad->generator = _get_generator(ad);
	}

	/* only 12 format */
	if (!i18n_ustring_copy_ua_n(u_pattern, "H:mm", sizeof(u_pattern))) {
		_E("ustring_copy() is failed.");
		return NULL;
	}

	u_best_pattern_capacity =
			(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));

	status = i18n_udatepg_get_best_pattern(ad->generator, u_pattern, i18n_ustring_get_length(u_pattern),
								u_best_pattern, u_best_pattern_capacity, &best_pattern_len);
	if(status != I18N_ERROR_NONE) {
		_E("get best pattern() failed(%d)", status);
		return NULL;
	}

	/* remove am/pm of best pattern */
	retv_if(!i18n_ustring_copy_au(a_best_pattern, u_best_pattern), NULL);
	_D("best pattern [%s]", a_best_pattern);
	a_best_pattern_fixed = strtok(a_best_pattern, "a");
	a_best_pattern_fixed = strtok(a_best_pattern_fixed, " ");
	_D("best pattern fixed [%s]", a_best_pattern_fixed);

	if (a_best_pattern_fixed) {
		/* exception - pt_BR(HH'h'mm), id_ID, da_DK */
		if (strncmp(a_best_pattern_fixed, "HH'h'mm", 7) == 0
			|| strncmp(ad->timeregion_format, "id_ID", 5) == 0
			|| strncmp(ad->timeregion_format, "da_DK", 5) == 0
			|| strncmp(ad->timeregion_format, "mr_IN", 5) == 0) {

			char *a_best_pattern_changed = g_strndup("HH:mm", 5);
			_D("best pattern is changed [%s]", a_best_pattern_changed);
			if(a_best_pattern_changed) {
				i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_changed);
				g_free(a_best_pattern_changed);
			}
		}
		else {
			retv_if(!i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_fixed), NULL);
		}
	}

	/* change char to UChar */

	retv_if(!i18n_ustring_copy_n(u_pattern, u_best_pattern, sizeof(u_pattern)), NULL);

	/* get formatter */
	i18n_ustring_copy_ua_n(u_timezone, ad->timezone_id, sizeof(u_timezone));
	status = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, ad->timeregion_format, u_timezone, -1,
							u_pattern, -1, &formatter);
	if (!formatter) {
		_E("time24_create() is failed.");
		return NULL;
	}

	_D("getting time formatter success");
	return formatter;
}



static i18n_udate_format_h _get_date_formatter(void *data)
{
	_D("");

	i18n_uchar u_timezone[64] = {0,};
	i18n_uchar u_skeleton[64] = {0,};
	int32_t skeleton_len = 0;
	int status = I18N_ERROR_INVALID_PARAMETER;

	i18n_uchar u_best_pattern[64] = {0,};
	int32_t u_best_pattern_capacity;
	i18n_udate_format_h formatter = NULL;

	appdata *ad = data;
	retv_if(!ad, NULL);

	i18n_ustring_copy_ua_n(u_skeleton, "MMMEd", strlen("MMMEd"));
	skeleton_len = i18n_ustring_get_length(u_skeleton);

	u_best_pattern_capacity =
					(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));
	status = i18n_udatepg_get_best_pattern(ad->generator, u_skeleton, skeleton_len,
								u_best_pattern, u_best_pattern_capacity, &status);
	if(status != I18N_ERROR_NONE) {
		_E("get best pattern() failed(%d)", status);
		return NULL;
	}

	if(strncmp(ad->timeregion_format, "fi_FI", 5) == 0) {
		char *a_best_pattern_changed = g_strndup("ccc, d. MMM", 11);
		_D("date formatter best pattern is changed [%s]", a_best_pattern_changed);
		if(a_best_pattern_changed) {
			i18n_ustring_copy_ua(u_best_pattern, a_best_pattern_changed);
			g_free(a_best_pattern_changed);
		}
	}

	/* get formatter */

	i18n_ustring_copy_ua_n(u_timezone, ad->timezone_id, sizeof(u_timezone));
	status = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, ad->timeregion_format, u_timezone, -1, u_best_pattern, -1, &formatter);

	if (!formatter) {
		_E("udate_create() is failed.");
		return NULL;
	}

	_D("getting date formatter success");

	return formatter;

}



static i18n_udate_format_h _get_ampm_formatter(void *data)
{
	_D("");

	int status = I18N_ERROR_INVALID_PARAMETER;

	char a_best_pattern[64] = {0.};

	i18n_uchar u_timezone[64] = {0,};
	i18n_uchar u_skeleton[64] = {0,};
	int32_t skeleton_len = 0;

	i18n_uchar u_best_pattern[64] = {0,};
	int32_t u_best_pattern_capacity;
	i18n_udate_format_h formatter = NULL;

	appdata *ad = data;
	retv_if(!ad, NULL);

	i18n_ustring_copy_ua_n(u_skeleton, "hhmm", strlen("hhmm"));
	skeleton_len = i18n_ustring_get_length(u_skeleton);

	u_best_pattern_capacity =
					(int32_t) (sizeof(u_best_pattern) / sizeof((u_best_pattern)[0]));
	status = i18n_udatepg_get_best_pattern(ad->generator, u_skeleton, skeleton_len,
								u_best_pattern, u_best_pattern_capacity, &status);
	if(status != I18N_ERROR_NONE) {
		_E("get best pattern() failed(%d)", status);
		return NULL;
	}

	i18n_ustring_copy_au(a_best_pattern, u_best_pattern);
	i18n_ustring_copy_ua(u_best_pattern, "a");

	if(a_best_pattern[0] == 'a') {
		ad->is_pre = EINA_TRUE;
	} else {
		ad->is_pre = EINA_FALSE;
	}

	/* get formatter */
	i18n_ustring_copy_ua_n(u_timezone, ad->timezone_id, sizeof(u_timezone));
	status = i18n_udate_create(I18N_UDATE_PATTERN, I18N_UDATE_PATTERN, ad->timeregion_format, u_timezone, -1, u_best_pattern, -1, &formatter);
	if (!formatter) {
		_E("ampm_create() is failed.");
		return NULL;
	}

	_D("getting ampm formatter success");

	return formatter;
}



static void _set_formatters(void *data)
{
	_D("");

	appdata *ad = data;
	ret_if(!ad);

	/* generator */
	ad->generator = _get_generator(ad);
	/* time formatter */
	ad->formatter_time = _get_time_formatter(ad);
	/* ampm formatter */
	ad->formatter_ampm = _get_ampm_formatter(ad);
	/* 24 time formatter */
	ad->formatter_time_24 = _get_time_formatter_24(ad);
	/* date formatter */
	ad->formatter_date = _get_date_formatter(ad);
}



static void _remove_formatters(void *data)
{
	_D("");

	appdata *ad = data;
	ret_if(!ad);

	if(ad->generator) {
		i18n_udatepg_destroy(ad->generator);
		ad->generator = NULL;
	}
	if(ad->formatter_time) {
		i18n_udate_destroy(ad->formatter_time);
		ad->formatter_time = NULL;
	}
	if(ad->formatter_ampm) {
		i18n_udate_destroy(ad->formatter_ampm);
		ad->formatter_ampm = NULL;
	}
	if(ad->formatter_time_24) {
		i18n_udate_destroy(ad->formatter_time_24);
		ad->formatter_time_24 = NULL;
	}
	if(ad->formatter_date) {
		i18n_udate_destroy(ad->formatter_date);
		ad->formatter_date = NULL;
	}
}



static int _get_formatted_date_from_utc_time(void *data, time_t intime, char *buf, int buf_len)
{
	appdata *ad = data;
	retv_if(!ad, -1);
	retv_if(!ad->formatter_date, -1);

	i18n_udate u_time = (i18n_udate)intime * 1000;
	i18n_uchar u_formatted_str[64] = {0,};
	int32_t u_formatted_str_capacity;
	int32_t formatted_str_len = -1;
	int status = I18N_ERROR_INVALID_PARAMETER;

	/* calculate formatted string capacity */
	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	/* fomatting date using formatter */
	status = i18n_udate_format_date(ad->formatter_date, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	if(status != I18N_ERROR_NONE) {
		_E("udat_format() failed");
		return -1;
	}

	if(formatted_str_len <= 0) {
		_E("formatted_str_len is less than 0");
	}

	buf = i18n_ustring_copy_au_n(buf, u_formatted_str, (int32_t)buf_len);
	_SD("date:(%d)[%s][%d]", formatted_str_len, buf, intime);

	return 0;
}



static int _get_formatted_ampm_from_utc_time(void *data, time_t intime, char *buf, int buf_len, int *ampm_len)
{
	appdata *ad = data;
	retv_if(!ad, -1);
	retv_if(!ad->formatter_ampm, -1);

	i18n_udate u_time = (i18n_udate)intime * 1000;
	i18n_uchar u_formatted_str[64] = {0,};
	int32_t u_formatted_str_capacity;
	int32_t formatted_str_len = -1;
	int status = I18N_ERROR_INVALID_PARAMETER;

	/* calculate formatted string capacity */
	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	/* fomatting date using formatter */
	status = i18n_udate_format_date(ad->formatter_ampm, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	if(status != I18N_ERROR_NONE) {
		_E("udat_format() failed");
		return -1;
	}

	if(formatted_str_len <= 0) {
		_E("formatted_str_len is less than 0");
	}


	(*ampm_len) = i18n_ustring_get_length(u_formatted_str);

	buf = i18n_ustring_copy_au_n(buf, u_formatted_str, (int32_t)buf_len);
	_SD("ampm:(%d)[%s][%d]", formatted_str_len, buf, intime);

	return 0;
}



static int _get_formatted_time_from_utc_time(void *data, time_t intime, char *buf, int buf_len, Eina_Bool is_time_24)
{
	appdata *ad = data;
	retv_if(!ad, -1);

	i18n_udate u_time = (i18n_udate)intime * 1000;
	i18n_uchar u_formatted_str[64] = {0,};
	int32_t u_formatted_str_capacity;
	int32_t formatted_str_len = -1;
	int status = I18N_ERROR_INVALID_PARAMETER;

	/* calculate formatted string capacity */
	u_formatted_str_capacity =
			(int32_t)(sizeof(u_formatted_str) / sizeof((u_formatted_str)[0]));

	/* fomatting date using formatter */
	if(is_time_24) {
		retv_if(ad->formatter_time_24 == NULL, -1);
		status = i18n_udate_format_date(ad->formatter_time_24, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	} else {
		retv_if(ad->formatter_time == NULL, -1);
		status = i18n_udate_format_date(ad->formatter_time, u_time, u_formatted_str, u_formatted_str_capacity, NULL, &formatted_str_len);
	}
	if(status != I18N_ERROR_NONE) {
		_E("udat_format() failed");
		return -1;
	}

	if(formatted_str_len <= 0)
		_E("formatted_str_len is less than 0");

	buf = i18n_ustring_copy_au_n(buf, u_formatted_str, (int32_t)buf_len);
	_SD("time:(%d)[%s][%d]", formatted_str_len, buf, intime);

	return 0;
}



#if 0
static char *_replaceAll(char *s, const char *olds, const char *news)
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
	if (!result) return NULL;

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
#endif



Eina_Bool clock_view_set_info_time(void *data)
{
	appdata *ad = data;
	if (!ad) {
		_D("appdata is NULL");
		return ECORE_CALLBACK_RENEW;
	}

	struct tm *ts = NULL;
	time_t tt;
	int err_code = 0;
	int showdate = 1;
	int clock_font_color = 8;
	display_state_e val;
	int ampm_length = 0;
	char font_buf[512] = {0, };
	char utc_date[256] = { 0, }; //text_date
	char utc_time[BUFFER_LENGTH] = { 0 };
	char utc_ampm[BUFFER_LENGTH] = { 0 };
	char *time_str = NULL;

	tt = time(NULL);
	ts = localtime(&tt);
	retv_if(!ts, ECORE_CALLBACK_RENEW);

	if (ad->timer != NULL) {
		ecore_timer_del(ad->timer);
		ad->timer = NULL;
	}

	device_display_get_state(&val);
	if(val != DISPLAY_STATE_SCREEN_OFF) {
		ad->timer = ecore_timer_add(60 - ts->tm_sec, clock_view_set_info_time, ad);
	}

	/* text_date */
	err_code = preference_get_int(DIGITAL_PREFERENCE_CLOCK_FONT_COLOR, &clock_font_color);
	if (0 != err_code) clock_font_color = setting_items_s.clock_font_color;

	if (clock_font_color < 1 || clock_font_color > 12) //Exception
		clock_font_color = 8;

	err_code = preference_get_int(DIGITAL_PREFERENCE_SHOW_DATE, &showdate);
	if (0 != err_code) showdate = setting_items_s.show_date;

	_D("show_date:%d", showdate);

	if (showdate) {
		_get_formatted_date_from_utc_time(ad, tt, utc_date, sizeof(utc_date));
		elm_object_part_text_set(ad->ly_main, "default_text_date", utc_date);
		snprintf(font_buf, sizeof(font_buf)-1, "%s_%d", "show,default_text_date", clock_font_color);
		elm_object_signal_emit(ad->ly_main, font_buf, "source_default_text_date");
	}
	_D("");

	Eina_Bool is_pre = ad->is_pre;
	Eina_Bool is_24hour = EINA_FALSE;

	if(!ad->timeformat) {
		_get_formatted_ampm_from_utc_time(ad, tt, utc_ampm, sizeof(utc_ampm), &ampm_length);
		_get_formatted_time_from_utc_time(ad, tt, utc_time, sizeof(utc_time), EINA_FALSE);
		is_24hour = EINA_FALSE;
	} else {
		_get_formatted_time_from_utc_time(ad, tt, utc_time, sizeof(utc_time), EINA_TRUE);
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

	if(is_24hour == EINA_TRUE){
		time_str = g_strdup_printf("<color=#%sFF>%s</color>", font_color_list[clock_font_color-1], utc_time);
	}
	else {
		if(is_pre== EINA_TRUE)
			time_str = g_strdup_printf("<color=#%sFF><font_size=24><font=Tizen:style=Bold>%s</font></font_size>%s</color>", font_color_list[clock_font_color-1], utc_ampm, utc_time);
		else
			// Todo : Fix the gap between clock and ampm.
			time_str = g_strdup_printf("<color=#%sFF>%s<font_size=24><font=Tizen:style=Bold> %s</font></font_size></color>", font_color_list[clock_font_color-1], utc_time, utc_ampm);
	}
	_D("time_str=%s", time_str);

	elm_object_part_text_set(ad->ly_main, "textblock_time", time_str);
	elm_object_signal_emit(ad->ly_main, "change,default", "source_textblock_time");

	g_free(time_str);

	return ECORE_CALLBACK_RENEW;
}



void clock_view_update_view(void *data)
{
	appdata *ad = data;
	if (!ad) {
		_E("appdata is NULL");
		return;
	}

	if(ad->win) {
		clock_view_set_info_time(ad);
	}
}



static i18n_uchar *_uastrcpy(const char *chars)
{
	int len = 0;
	i18n_uchar *str = NULL;
	len = strlen(chars);
	str = (i18n_uchar *) malloc(sizeof(i18n_uchar) *(len + 1));
	if (!str)
		return NULL;
	i18n_ustring_copy_ua(str, chars);
	return str;
}



static void ICU_set_timezone(const char *timezone)
{
	_D("%s", __func__);
	if(!timezone) {
		_E("TIMEZONE is NULL");
		return;
	}

	int ec = I18N_ERROR_NONE;
	i18n_uchar *str = _uastrcpy(timezone);

	ec = i18n_ucalendar_set_default_timezone(str);
	if (ec != I18N_ERROR_NONE) {
	} else {
		_E("ucal_setDefaultTimeZone() FAILED");
	}
	free(str);
}



static char* _get_timezone()
{
	char *timezone = NULL;

	int ret = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_TIMEZONE, &timezone);
	if (ret < 0) {
		_E("fail to get the locale time zone value(%d)", ret);
	}

	if(!timezone) {
		_E("system settings fail to get value: time zone");
	}

	_D("time zone : %s", timezone);

	return timezone;
}



static void _time_status_changed_cb(system_settings_key_e key, void *data)
{
	_D("");

	appdata *ad = data;
	ret_if(!ad);

	if(ad->timezone_id) {
		free(ad->timezone_id);
		ad->timezone_id = NULL;
	}
	if(ad->timeregion_format) {
		free(ad->timeregion_format);
		ad->timeregion_format = NULL;
	}

	ad->timezone_id = _get_timezone();
	ICU_set_timezone(ad->timezone_id);
	ad->timeregion_format = _get_locale();

	_D("[%d][%s][%s]", ad->timeformat, ad->timeregion_format, ad->timezone_id);
	_D("system setting key : %d", key);

	_remove_formatters(ad);
	_set_formatters(ad);

	clock_view_set_info_time(ad);
}



static void _clear_time(void *data)
{
	_D("");
	appdata *ad = data;
	ret_if(!ad);

	clock_view_hide_clock(ad);

	if (ad->timer) {
		ecore_timer_del(ad->timer);
		ad->timer = NULL;
	}
}



static void _clock_font_changed_cb(int node, void *data)
{
	appdata *ad = data;
	ret_if(!ad);

	char font_buf[512] = {0, };

	int show_date = 1;
	int clock_font_color = 8;
	int ret = 0;

	/* font */
	elm_config_font_overlay_set("idle_font", FONT_DEFAULT_FAMILY_NAME, FONT_DEFAULT_SIZE);

	elm_config_font_overlay_apply();

	ret = preference_get_int(DIGITAL_PREFERENCE_SHOW_DATE, &setting_items_s.show_date);
	if (!ret) show_date = setting_items_s.show_date;
	else _E("fail to get the show date (%d)", ret);

	ret = preference_get_int(DIGITAL_PREFERENCE_CLOCK_FONT_COLOR, &setting_items_s.clock_font_color);
	if (!ret) clock_font_color = setting_items_s.clock_font_color;
	else _E("fail to get the clock font color (%d)", ret);

	_D("show_date:%d, font color:%d", show_date, clock_font_color);

	if(show_date) {
		snprintf(font_buf, sizeof(font_buf)-1, "%s_%d", "show,default_text_date", clock_font_color);
		elm_object_signal_emit(ad->ly_main, font_buf, "source_default_text_date");
		elm_object_signal_emit(ad->ly_main, "change,default", "source_textblock_time");
	}
	else {
		elm_object_signal_emit(ad->ly_main, "hide,text_date", "source_text_date");
		elm_object_signal_emit(ad->ly_main, "change,no_data", "source_textblock_time");
	}

	if(node) {
		clock_view_set_info_time(ad);
	}
}



int clock_view_get_display_state()
{
	display_state_e val;
	device_display_get_state(&val);
	_D("DISPLAY STATE [%d]", val);
	return val;
}



static void _device_state_changed_cb(device_callback_e type, void *value, void *data)
{
	appdata *ad = data;
	ret_if(!ad);

	struct tm *ts = NULL;
	time_t tt;

	display_state_e val;
	device_display_get_state(&val);
	_D("DISPLAY STATE [%d]", val);

	if(val == DISPLAY_STATE_NORMAL) {
		if(!ad->is_show) {
			clock_view_set_info_time(ad);
			clock_view_show_clock(ad);
		}

		tt = time(NULL);
		ret_if(tt == (time_t)-1);
		ts = localtime(&tt);
		ret_if(!ts);
		if (ad->timer) {
			ecore_timer_del(ad->timer);
			ad->timer = NULL;
		}
		ad->timer = ecore_timer_add(60 - ts->tm_sec, clock_view_set_info_time, ad);
	}
	else if(val == DISPLAY_STATE_SCREEN_OFF) {
		//_clear_time(data);	//Disable this code for transit to alpm clock
	}
	else
		_D("Not interested PM STATE");
}



static void _language_changed_cb(system_settings_key_e key, void *data)
{
	_D("%s", __func__);
	appdata *ad = data;
	ret_if(!ad);

	sleep(1);
	_time_status_changed_cb(key, ad);
}



static void _timeformat_changed_cb(system_settings_key_e key, void *data)
{
	_D("");
	appdata *ad = data;
	ret_if(!ad);

	bool val = false;

	system_settings_get_value_bool(key, &val);
	ad->timeformat = (int) val;
	_time_status_changed_cb(key, ad);
}



static void _set_settings(void *data)
{
	_D("");
	appdata *ad = data;
	ret_if(!ad);
	int ret = -1;

	/* register time changed cb */
	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_TIME_CHANGED, _time_status_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to set time changed cb.(%d)", ret);
	}
	/*Register changed cb(timezone) */
	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEZONE, _time_status_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to set time zone change cb(%d)", ret);
	}
	/* Register changed cb(pm mode) */
	ret = device_add_callback(DEVICE_CALLBACK_DISPLAY_STATE, _device_state_changed_cb, ad);
	if(DEVICE_ERROR_NONE != ret) {
		_E("Failed to add device display state changed cb(%d)", ret);
	}

	/*Register changed cb(language) */
	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, _language_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to set language setting's changed cb.(%d)", ret);
	}

	/*Register changed cb(regionformat 1224) */
	ret = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, _timeformat_changed_cb, ad);
	if(ret < 0) {
		_E("Failed to set time format about 24 hour changed cb(%d).", ret);
	}
}



static void _unset_settings()
{
	int ret = -1;

	/* unset changed cb(time changed) */
	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_TIME_CHANGED);
	if(ret < 0) {
		_E("Failed to unset time changed(%d).", ret);
	}
	/* unset changed cb(timezone) */
	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEZONE);
	if(ret < 0) {
		_E("Failed to unset time zone changed cb(%d).", ret);
	}
	/* unset changed cb(pm mode) */
	ret = device_remove_callback(DEVICE_CALLBACK_DISPLAY_STATE, _device_state_changed_cb);
	if(DEVICE_ERROR_NONE != ret) {
		_E("Failed to remove device display state changed cb(%d)", ret);
	}

	/* unset changed cb(language)*/
	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE);
	if(ret < 0) {
		_E("Failed to unset locale language(%d).", ret);
	}

	/* unset changed cb(region format)*/
	ret = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
	if(ret < 0) {
		_E("Failed to unset locale time format about 24 hour(%d).", ret);
	}

	/* date showing */
	ret = preference_unset_changed_cb(DIGITAL_PREFERENCE_SHOW_DATE);
	if(ret < 0) {
		_E("Failed to unset preference(show_date, %d).", ret);
	}

	/* clock_font_color */
	ret = preference_unset_changed_cb(DIGITAL_PREFERENCE_CLOCK_FONT_COLOR);
	if(ret < 0) {
		_E("Failed to unset preference(font color, %d).", ret);
	}
}



static void _set_info(void *data)
{
	_D("");

	appdata *ad = data;
	ret_if(!ad);

	_set_settings(data);

	_time_status_changed_cb(-1, ad);
	_clock_font_changed_cb(0, ad);

	clock_view_set_info_time(ad);
}



static void on_changed_receive(void *data, DBusMessage *msg)
{
	appdata *ad = (appdata *)data;
	ret_if(!ad);

	int lcd_on = 0;
	int lcd_off = 0;

	lcd_on = dbus_message_is_signal(msg, DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_ON);
	lcd_off = dbus_message_is_signal(msg, DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_OFF);

	if(lcd_on) {
		_D("%s - %s", DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_ON);
		clock_view_set_info_time(ad);
		clock_view_show_clock(ad);
	} else if(lcd_off) {
		_D("%s - %s", DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_OFF);

	} else {
		_E("%s - %s", DEVICED_PROCESS_INTERFACE, "unknown signal");
	}
}



static void idle_clock_digital_register_lcd_onoff(void *data)
{
	E_DBus_Signal_Handler *handler = NULL;
	E_DBus_Connection *conn = NULL;

	// Init
	ecore_init();
	e_dbus_init();

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (!conn) {
		_E("e_dbus_bus_get error");
			return;
	}

	handler = e_dbus_signal_handler_add(conn, NULL, DEVICED_PROCESS_PATH,
										DEVICED_PROCESS_INTERFACE, SIGNAL_LCD_ON,
										on_changed_receive, data);
	if (!handler) {
			_E("e_dbus_signal_handler_add error");
			return;
	}
}



static void _ecore_evas_msg_parent_handle(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
	_D("Receive msg from clien msg_domain=%x msg_id=%x size=%d", msg_domain, msg_id, size);
	_D(" ########## data : %s", data);

	if(g_strcmp0(data, "mc_resume") == 0) {
		_D("##### mc_resume");
	} else if(g_strcmp0(data, "mc_pause") == 0) {
		_D("##### mc_pause");
	}
}



static Evas_Object *_add_layout(Evas_Object *parent, const char *file, const char *group)
{
	_D("%s", __func__);
	Evas_Object *eo = NULL;
	int r = -1;

	retv_if(!parent, NULL);
	retv_if(!file, NULL);
	retv_if(!group, NULL);

	eo = elm_layout_add(parent);
	retv_if(!eo, NULL);

	r = elm_layout_file_set(eo, file, group);
	if (!r) {
		_E("Failed to set file[%s]", file);
		evas_object_del(eo);
		return NULL;
	}

	evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(eo);
	return eo;
}



void clock_view_destroy_view_main(void *data)
{
	appdata *ad = data;
	ret_if(!ad);

	if(ad->timezone_id) {
		free(ad->timezone_id);
		ad->timezone_id = NULL;
	}
	if(ad->timeregion_format) {
		free(ad->timeregion_format);
		ad->timeregion_format = NULL;
	}

	_remove_formatters(ad);
}



bool clock_view_create_layout(void *data)
{
	appdata *ad = data;
	retv_if(!ad, false);

	/* create main layout */
	Evas_Object *ly_main = NULL;
	Evas *evas = NULL;
	Ecore_Evas *ee = NULL;

	ly_main = _add_layout(ad->win, EDJ_APP, "layout_clock_digital");
	retv_if(!ly_main, -1);
	evas_object_size_hint_weight_set(ly_main, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ly_main);
	ad->ly_main = ly_main;
	evas_object_show(ad->win);

	evas = evas_object_evas_get(ad->win);
	ee = ecore_evas_ecore_evas_get(evas);
	ecore_evas_callback_msg_parent_handle_set(ee, _ecore_evas_msg_parent_handle);


	evas_object_resize(ly_main, ad->win_w, ad->win_h);
	evas_object_show(ly_main);

	idle_clock_digital_register_lcd_onoff(ad);

	_set_info(ad);

	return true;
}



