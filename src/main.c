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
#include <glib.h>
#include <dd-display.h>
#include <device/display.h>
#include <tizen.h>
#include <watch_app.h>
#include <watch_app_efl.h>


#include "log.h"
#include "window.h"
#include "app_data.h"
#include "clock_view.h"
#include "util.h"



#define DATA_PATH	"/tmp"
#define DUMP_FILE_PATH_OFFSCREEN		DATA_PATH"/"PACKAGE_NAME"-dump_offscreen.png"
#define DUMP_FILE_PATH_MINICONTROL		DATA_PATH"/"PACKAGE_NAME"-dump_minicontrol.png"
#define QUALITY_N_COMPRESS "quality=100 compress=1"

#define WALLPAPER_PATH "/opt/usr/share/settings/Wallpapers/"



static int drawing_state = 0; // 0: nothing, 1: offscreen capture ongoing, 2: onscreen capture ongoing
static Ecore_Timer *sync_timer = NULL;
static Ecore_Timer *drawing_timer = NULL;
static Ecore_Timer *close_timer = NULL;

static Evas *offscreen_e = NULL;
static Evas_Object *offscreen_box = NULL;
static Evas_Object *offscreen_bg = NULL;
static Evas_Object *offscreen_img = NULL;
static Evas *minicontrol_e = NULL;
static Evas_Object *minicontrol_bg = NULL;
static Evas_Object *minicontrol_img = NULL;
static Evas_Object *minicontrol_img_box = NULL;

static Evas* _live_create_virtual_canvas(int w, int h);
static int live_flush_to_file(Evas *e, const char *filename, int w, int h);
static int _flush_data_to_file(Evas *e, char *data, const char *filename, int w, int h);
static void _update_clock_to_offscreen(void *data, int w, int h);

EXPORT_API Evas_Object* elm_widget_top_get(const Evas_Object *obj);
EXPORT_API Evas_Object* elm_widget_parent_widget_get(const Evas_Object *obj);



static void _live_destroy_virtual_canvas(Evas *e)
{
	ret_if(!e);
	Ecore_Evas *ee;

	ee = ecore_evas_ecore_evas_get(e);
	if (!ee) {
		_E("Failed to ecore evas object\n");
		return ;
	}

	ecore_evas_free(ee);
}



void _remove_preview_resource(void *data)
{
	_D();

	appdata *ad = data;
	ret_if(!ad);

	if(offscreen_box) {
		evas_object_del(offscreen_box);
		offscreen_box = NULL;
		ad->win = NULL;
		_D("##### ad->win set to NULL");
	}

	if(offscreen_bg) {
		evas_object_del(offscreen_bg);
		offscreen_bg = NULL;
	}

	if(offscreen_img) {
		evas_object_del(offscreen_img);
		offscreen_img = NULL;
	}

	if(minicontrol_bg) {
		evas_object_del(minicontrol_bg);
		minicontrol_bg = NULL;
	}

	if(minicontrol_img) {
		evas_object_del(minicontrol_img);
		minicontrol_img = NULL;
	}

	if(minicontrol_img_box) {
		evas_object_del(minicontrol_img_box);
		minicontrol_img_box = NULL;
	}

	_live_destroy_virtual_canvas(offscreen_e);
	_live_destroy_virtual_canvas(minicontrol_e);

	ad->e_offscreen = NULL;
}



static void _update_clock_to_offscreen(void *data, int w, int h)
{
	appdata *ad = data;
	ret_if(!ad);

	if(!offscreen_e) {
		offscreen_e = _live_create_virtual_canvas(w, h);
		ad->e_offscreen = offscreen_e;
	}

	offscreen_box = evas_object_rectangle_add(offscreen_e);
	evas_object_resize(offscreen_box, w, h);
	evas_object_color_set(offscreen_box, 0, 0, 0, 0);
	evas_object_show(offscreen_box);

	ad->win = offscreen_box;

	clock_view_create_layout(ad);
	_D("create offscreen window");

}



static Evas* _live_create_virtual_canvas(int w, int h)
{
	Ecore_Evas *internal_ee;
	Evas *internal_e;

	internal_ee = ecore_evas_buffer_new(w, h);
	if (!internal_ee) {
		_E("Failed to create a new canvas buffer");
		return NULL;
	}

	ecore_evas_alpha_set(internal_ee, EINA_FALSE);
	ecore_evas_manual_render_set(internal_ee, EINA_TRUE);

	internal_e = ecore_evas_get(internal_ee);
	if (!internal_e) {
		ecore_evas_free(internal_ee);
		_E("Faield to get Evas object");
		return NULL;
	}

	return internal_e;
}



static int live_flush_to_file(Evas *e, const char *filename, int w, int h)
{
	void *data;
	Ecore_Evas *internal_ee;

	internal_ee = ecore_evas_ecore_evas_get(e);
	if (!internal_ee) {
		_E("Failed to get ecore evas");
		return -1;
	}

	ecore_evas_manual_render(internal_ee);
	// Get a pointer of a buffer of the virtual canvas
	data = (void*)ecore_evas_buffer_pixels_get(internal_ee);

	if (!data) {
		_E("Failed to get pixel data");
		return -1;
	}

	return _flush_data_to_file(e, (char *) data, filename, w, h);
}



static int _flush_data_to_file(Evas *e, char *data, const char *filename, int w, int h)
{
	Evas_Object *output = NULL;

	output = evas_object_image_add(e);
	if (!output) {
		_E("Failed to create an image object");
		return -1;
	}

	/* evas_object_image_data_get/set should be used as pair. */
	evas_object_image_colorspace_set(output, EVAS_COLORSPACE_ARGB8888);
	// evas_object_image_alpha_set(output, EINA_FALSE);
	evas_object_image_alpha_set(output, EINA_TRUE);
	evas_object_image_size_set(output, w, h);
	evas_object_image_smooth_scale_set(output, EINA_TRUE);
	evas_object_image_data_set(output, data);
	evas_object_image_data_update_add(output, 0, 0, w, h);

	if (evas_object_image_save(output, filename, NULL, QUALITY_N_COMPRESS) == EINA_FALSE) {
		evas_object_del(output);
		_E("Faield to save a captured image (%s)", filename);
		return -1;
	}

	evas_object_del(output);

	return 0;
}



static Ecore_X_Window _x11_elm_widget_xwin_get(const Evas_Object *obj)
{
	Evas_Object *top;
	Ecore_X_Window xwin = 0;

	top = elm_widget_top_get(obj);
	if (!top) top = elm_widget_top_get(elm_widget_parent_widget_get(obj));
	if (top) xwin = elm_win_xwindow_get(top);
	return xwin;
}



static Eina_Bool _make_dump(void *data, int win_type, int width, int height, char *file)
{
	appdata *ad = data;
	Evas_Object *obj = NULL;

	Ecore_X_Window xwin = 0;
	Evas *e = NULL;
	Ecore_Evas *ee = NULL;
	const void *pixel_data = NULL;

	retv_if(!ad, EINA_FALSE);
	retv_if(!win_type, EINA_FALSE);

	obj = ad->win;

	_D("obj: %x", obj);

	if(BUFFER_TYPE_WINDOW == win_type) {
		// For screen capture
		// xwin = ecore_x_window_root_get(obj);
		xwin = _x11_elm_widget_xwin_get(obj);
		_D("xwin: %x", xwin);
	}

	_flush_data_to_file(e, (char *)pixel_data, DUMP_FILE_PATH_MINICONTROL, 384, WIN_SIZE_H);

	return EINA_TRUE;
}



static bool _create_window(void *data, int mode)
{
	Evas_Object *win = NULL;
	appdata *ad = data;
	retv_if(!ad, false);

	/* create main window */
	if(ad->win == NULL) {
		if(mode == BUFFER_TYPE_OFFSCREEN) {
			ad->win_type = BUFFER_TYPE_OFFSCREEN;
			_update_clock_to_offscreen(data, ad->win_w, ad->win_h);

			return true;
		}
		win = window_create(PACKAGE);
		retv_if(!win, -1);
		evas_object_resize(win, ad->win_w, ad->win_h);
		evas_object_move(win, 0, 0);
		ad->win = win;
		ad->win_type = BUFFER_TYPE_WINDOW;
		clock_view_create_layout(ad);
		_D("create window");
	}
	return true;
}



static bool _idle_clock_digital_create(int width, int height, void *data)
{
	_D("%s", __func__);

	appdata *ad = data;

	app_event_handler_h handlers[5] = {NULL, };
	watch_time_h watch_time = NULL;

	// Register callbacks for each system event
	if (watch_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, NULL, NULL) != APP_ERROR_NONE) {
		 _E("watch_app_add_event_handler () is failed");
	}
	if (watch_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, NULL, NULL) != APP_ERROR_NONE) {
		 _E("watch_app_add_event_handler () is failed");
	}
	if (watch_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, NULL, NULL) != APP_ERROR_NONE) {
		 _E("watch_app_add_event_handler () is failed");
	}
	if (watch_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, NULL, NULL) != APP_ERROR_NONE) {
		 _E("watch_app_add_event_handler () is failed");
	}
	if (watch_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, NULL, NULL) != APP_ERROR_NONE) {
		 _E("watch_app_add_event_handler () is failed");
	}

	ad->win_w = width;
	ad->win_h = height;

	return true;
}



static void _idle_clock_digital_terminate(void *data)
{
	_D();
	appdata *ad = data;

	if (ad->win) {
		evas_object_del(ad->win);
		ad->win = NULL;
	}

	if (ad->ly_main) {
		evas_object_del(ad->ly_main);
		ad->ly_main = NULL;
	}

	if(close_timer) {
		ecore_timer_del(close_timer);
		close_timer = NULL;
	}

	clock_view_destroy_view_main(ad);

}



static void _idle_clock_digital_pause(void *data)
{
	_D("%s", __func__);
	appdata *ad = data;
	ret_if(!ad);
}



static void _idle_clock_digital_resume(void *data)
{
	_D("%s", __func__);
	appdata *ad = data;
	ret_if(!ad);
}



static void _send_reply(void *data)
{
	_D("%s", __func__);

	int ret;

	appdata *ad = data;
	ret_if(!ad);

	/* send reply */
	app_control_h reply;
	ret = app_control_create(&reply);
	_D("reply app_control created");
	if (ret == 0) {
		_D("reply caller");
		if(ad->win_type == BUFFER_TYPE_OFFSCREEN)
			ret = app_control_add_extra_data(reply, "result", DUMP_FILE_PATH_OFFSCREEN);
		else
			ret = app_control_add_extra_data(reply, "result", DUMP_FILE_PATH_MINICONTROL);

		if(ret)
			_E("app_control_add_extra_data failed");

		ret = app_control_reply_to_launch_request(reply, ad->app_control, APP_CONTROL_RESULT_SUCCEEDED);

		if(ret)
			_E("app_control_reply_to_launch_request failed");

		app_control_destroy(reply);
	} else {
		_E("app_control_create failed");
	}

}



static void _draw_onscreen(void *data)
{
	_D("");

	appdata *ad = data;
	ret_if(!ad);

	_make_dump(data, ad->win_type, WIN_SIZE_W, WIN_SIZE_H, DUMP_FILE_PATH_MINICONTROL);
	_send_reply(data);

	if(clock_view_get_display_state() == DISPLAY_STATE_SCREEN_OFF)
		clock_view_hide_clock(ad);

	drawing_state = 0;
	drawing_timer = NULL;
}



static Eina_Bool _idler_drawing_onscreen_cb(void *data)
{
	_D("");

	appdata *ad = data;
	retv_if(!ad, ECORE_CALLBACK_CANCEL);

	_draw_onscreen(ad);

	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _drawing_timer_onscreen_cb(void *data)
{
	appdata *ad = data;
	retv_if(!ad, ECORE_CALLBACK_CANCEL);

	if(!ad->is_show) {
		clock_view_show_clock(ad);
		ecore_idler_add(_idler_drawing_onscreen_cb, ad);
		return ECORE_CALLBACK_CANCEL;
	}

	_draw_onscreen(ad);

	return ECORE_CALLBACK_CANCEL;
}



static void _draw_offscreen(void *data)
{
	_D();

	appdata *ad = data;
	ret_if(!ad);

	live_flush_to_file(ad->e_offscreen, DUMP_FILE_PATH_OFFSCREEN, WIN_SIZE_W, WIN_SIZE_H);

	_send_reply(data);

	if(clock_view_get_display_state() == DISPLAY_STATE_SCREEN_OFF)
		clock_view_hide_clock(ad);

	drawing_state = 0;
	drawing_timer = NULL;

	_D("win type = %d and window = %d", ad->win_type, ad->win);
	if(BUFFER_TYPE_OFFSCREEN == ad->win_type)
	{
		_D("offscreen capture completed. app will be closed");
		elm_exit();
	}
}



static Eina_Bool _idler_drawing_offscreen_cb(void *data)
{
	_D();

	appdata *ad = data;
	retv_if(!ad, ECORE_CALLBACK_RENEW);

	_draw_offscreen(ad);

	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _drawing_timer_cb(void *data)
{
	_D();

	appdata *ad = data;
	retv_if(!ad, ECORE_CALLBACK_RENEW);

	if(!ad->is_show) {
		clock_view_show_clock(ad);
		ecore_idler_add(_idler_drawing_offscreen_cb, ad);
		return ECORE_CALLBACK_CANCEL;
	}

	_draw_offscreen(ad);

	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _close_timer_cb(void *data)
{
	_D();

	elm_exit();
	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _sync_timer_cb(void *data)
{
	_D();

	appdata *ad = data;
	retv_if(!ad, ECORE_CALLBACK_RENEW);

	if(drawing_state == 0) {
		_remove_preview_resource(data);
		_create_window(data, BUFFER_TYPE_WINDOW);
		sync_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	return ECORE_CALLBACK_RENEW;
}



static void _idle_clock_digital_app_control(app_control_h app_control, void *data)
{
	_D();

	appdata *ad = data;
	ret_if(!ad);

	char *op = NULL;
	char *result_data = NULL;

	app_control_get_operation(app_control, &op);

	if(op) {
		_D("operation:%s", op);
		if(strcmp(op, "http://tizen.org/appcontrol/operation/remote_settings") == 0) {
			app_control_get_extra_data(app_control, "http://tizen.org/appcontrol/data/result_xml", &result_data);
			if(result_data) {
				_D("Result:%s", result_data);
				clock_view_parse_result_data(result_data);
				clock_view_set_result_data(ad);
				clock_view_update_view(ad);
				free(result_data);

				if (BUFFER_TYPE_WINDOW == ad->win_type && ad->win) {
					display_change_state(LCD_NORMAL);
				} else {
					_D("app will be closed");
					if(close_timer) {
						ecore_timer_del(close_timer);
						close_timer = NULL;
					}
					close_timer = ecore_timer_add(3.0, _close_timer_cb, NULL);
				}
			}
		}
		else if(strcmp(op, "http://tizen.org/appcontrol/operation/main") == 0) {
			if(close_timer) {
				ecore_timer_del(close_timer);
				close_timer = NULL;
			}
			if(drawing_state) {
				if (sync_timer) {
					ecore_timer_del(sync_timer);
					sync_timer = NULL;
				}
				sync_timer = ecore_timer_add(0.1, _sync_timer_cb, data);
			} else {
				/* New case: offscreen capture -> operation/main */
				_remove_preview_resource(data);
				_create_window(data, BUFFER_TYPE_WINDOW); /* create window if not mini app setting called */
			}
		}
		else if(strcmp(op, "http://tizen.org/appcontrol/operation/clock/capture") == 0) {
			app_control_clone(&ad->app_control, app_control);

			if(close_timer) {
				ecore_timer_del(close_timer);
				close_timer = NULL;
			}

			_create_window(data, BUFFER_TYPE_OFFSCREEN);

			if(BUFFER_TYPE_OFFSCREEN == ad->win_type) {
				_D("offscreen capture");
				drawing_state = 1;

				if (drawing_timer) {
					ecore_timer_del(drawing_timer);
					drawing_timer = NULL;
				}
				clock_view_set_info_time(data);
				if(clock_view_get_display_state() == DISPLAY_STATE_SCREEN_OFF)
					clock_view_show_clock(ad);
				drawing_timer = ecore_timer_add(0.15, _drawing_timer_cb, data);
			} else {
				_D("minicontrol capture");
				drawing_state = 2;
				if (drawing_timer) {
					ecore_timer_del(drawing_timer);
					drawing_timer = NULL;
				}
				clock_view_set_info_time(data);
				if(clock_view_get_display_state() == DISPLAY_STATE_SCREEN_OFF)
					clock_view_show_clock(ad);
				drawing_timer = ecore_timer_add(0.15, _drawing_timer_onscreen_cb, data);
			}
		}
		else {
			_E("Unknown operation");
		}
		free(op);
	}
}



void app_time_tick(watch_time_h watch_time, void* user_data)
{
	appdata *ad = user_data;

}

void app_ambient_tick(watch_time_h watch_time, void* user_data)
{
	appdata *ad = user_data;

}

void app_ambient_changed(bool ambient_mode, void* user_data)
{
	if (ambient_mode) {
		// Prepare to enter the ambient mode
	} else {
		// Prepare to exit the ambient mode
	}
}



int main(int argc, char *argv[])
{
	appdata ad;

	watch_app_lifecycle_callback_s lifecycle_callback = {0, };

	lifecycle_callback.create = _idle_clock_digital_create;
	lifecycle_callback.terminate = _idle_clock_digital_terminate;
	lifecycle_callback.pause = _idle_clock_digital_pause;
	lifecycle_callback.resume = _idle_clock_digital_resume;
	lifecycle_callback.app_control = _idle_clock_digital_app_control;
	lifecycle_callback.time_tick = app_time_tick;
	lifecycle_callback.ambient_tick = app_ambient_tick;
	lifecycle_callback.ambient_changed = app_ambient_changed;


	memset(&ad, 0x0, sizeof(appdata));

	return watch_app_main(argc, argv, &lifecycle_callback, &ad);
}



