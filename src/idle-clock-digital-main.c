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
#include <vconf-keys.h>
#include <glib.h>
#include <vconf.h>
#include <dd-display.h>

#include "idle-clock-digital-debug.h"
#include "idle-clock-digital-window.h"
#include "idle-clock-digital-app-data.h"
#include "idle-clock-digital-view-main.h"
#include "idle-clock-digital-common.h"

#ifdef PROFILE
#include <stdio.h>
#include <sys/timeb.h>

#ifdef WIN32
#  define ftime _ftime
#  define timeb _timeb
#endif

typedef struct STOPWATCH
{
    struct timeb m_ftStart;
    struct timeb m_ftStop;
} STOPWATCH;

static STOPWATCH sw;

void Start(STOPWATCH *pStopWatch)
{
    ftime(&pStopWatch->m_ftStart);
}

void Stop(STOPWATCH *pStopWatch)
{
    ftime(&pStopWatch->m_ftStop);
}

unsigned long GetElapsedMM(STOPWATCH *pStopWatch)
{
    unsigned long ulRet = 0;

    ulRet = ((pStopWatch->m_ftStop.time - pStopWatch->m_ftStart.time) * 1000);
    ulRet += (pStopWatch->m_ftStop.millitm  - pStopWatch->m_ftStart.millitm);

    return ulRet;
}
#endif

#define DATA_PATH	"/tmp"
#define DUMP_FILE_PATH_OFFSCREEN		DATA_PATH"/"PACKAGE_NAME"-dump_offscreen.png"
#define DUMP_FILE_PATH_MINICONTROL		DATA_PATH"/"PACKAGE_NAME"-dump_minicontrol.png"
// #define DUMP_FILE_PATH_OFFSCREEN		"/opt/usr/media/dump_offscreen.png"
// #define DUMP_FILE_PATH_MINICONTROL		"/opt/usr/media/dump_minicontrol.png"
#define QUALITY_N_COMPRESS "quality=100 compress=1"

#define VCONF_BG_SET "db/menu_widget/bgset" // mode 2
#define VCONF_BG_WALLPAPER "db/wms/home_bg_wallpaper" // mode 1
#define VCONF_BG_PALETTE "db/wms/home_bg_palette" // mode 0
#define VCONF_BG_MODE "db/wms/home_bg_mode"
#define WALLPAPER_PATH "/opt/usr/share/settings/Wallpapers/"

static int drawing_state = 0; // 0: nothing, 1: offscreen capture ongoing, 2: onscreen capture ongoing
static Ecore_Timer *sync_timer = NULL;
static Ecore_Timer *drawing_timer = NULL;
static Ecore_Timer *close_timer = NULL;
static int bg_mode = 0;
static char *bg_set = NULL;
static char *bg_wallpaper = NULL;
static char *bg_palette = NULL;

static Evas *offscreen_e = NULL;
static Evas_Object *offscreen_box = NULL;
static Evas_Object *offscreen_bg = NULL;
static Evas_Object *offscreen_img = NULL;
static Evas *minicontrol_e = NULL;
static Evas_Object *minicontrol_bg = NULL;
static Evas_Object *minicontrol_img = NULL;
static Evas_Object *minicontrol_img_box = NULL;

static Evas* __live_create_virtual_canvas(int w, int h);
static int live_flush_to_file(Evas *e, const char *filename, int w, int h);
static int _flush_data_to_file(Evas *e, char *data, const char *filename, int w, int h);
static void __update_clock_to_offscreen(void *data, int w, int h);

EAPI Evas_Object* elm_widget_top_get(const Evas_Object *obj);
EAPI Evas_Object* elm_widget_parent_widget_get(const Evas_Object *obj);
Ecore_X_Window _elm_ee_xwin_get(const Ecore_Evas *ee);

/*
static void _cb(void *data, Evas *evas, void *event_info)
{
	_D("##### rendering is done");
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	live_flush_to_file(ad->e_offscreen, "/opt/usr/media/Downloads/dumpoff.png", 320, 320);

	evas_event_callback_del(evas, EVAS_CALLBACK_RENDER_FLUSH_POST, _cb);
}
*/

static void __live_destroy_virtual_canvas(Evas *e)
{
	ret_if(e == NULL);
	Ecore_Evas *ee;

	ee = ecore_evas_ecore_evas_get(e);
	if (!ee) {
		_E("Failed to ecore evas object\n");
		return ;
	}

	ecore_evas_free(ee);
}

void __remove_preview_resource(void *data)
{
	_D();
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

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

	__live_destroy_virtual_canvas(offscreen_e);
	__live_destroy_virtual_canvas(minicontrol_e);

	ad->e_offscreen = NULL;
}

static void __update_bg_info()
{
	char *wallpaper = NULL;

	if(bg_wallpaper) {
		g_free(bg_wallpaper);
		bg_wallpaper = NULL;
	}
	if(bg_palette) {
		free(bg_palette);
		bg_palette = NULL;
	}

	vconf_get_int(VCONF_BG_MODE, &bg_mode);
	wallpaper = vconf_get_str(VCONF_BG_WALLPAPER);
	bg_palette = vconf_get_str(VCONF_BG_PALETTE);
	bg_set = vconf_get_str(VCONF_BG_SET);

	bg_wallpaper = g_strdup_printf("%s%s", WALLPAPER_PATH, wallpaper);
	free(wallpaper);

	_D("##### bg_mode: %d, bg_set: %s, bg_wallpaper: %s, bg_palette: %s",
				bg_mode, bg_set ? bg_set:"", bg_wallpaper ? bg_wallpaper:"", bg_palette ? bg_palette:"");
}

static void __get_rgb_from_string(int *r, int *g, int *b)
{
	retm_if(bg_palette == NULL, "bg_palette is null. get rgb failed.");

	char s_r[3];
	char s_g[3];
	char s_b[3];

	sprintf(s_r, "%c%c", bg_palette[0], bg_palette[1]);
	sprintf(s_g, "%c%c", bg_palette[2], bg_palette[3]);
	sprintf(s_b, "%c%c", bg_palette[4], bg_palette[5]);

	*r = strtol(s_r, NULL, 16);
	*g = strtol(s_g, NULL, 16);
	*b = strtol(s_b, NULL, 16);

	_D("##### r: %d, g: %d, b: %d", *r, *g, *b);
}

static void __update_clock_to_offscreen(void *data, int w, int h)
{
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	int r = 0;
	int g = 0;
	int b = 0;

	if(!offscreen_e) {
		offscreen_e = __live_create_virtual_canvas(w, h);
		ad->e_offscreen = offscreen_e;
	}
#if 0
	if(bg_mode == 0) {
		if(!offscreen_bg)
			offscreen_bg = evas_object_rectangle_add(offscreen_e);

		evas_object_resize(offscreen_bg, w, h);
		__get_rgb_from_string(&r, &g, &b);
		evas_object_color_set(offscreen_bg, r, g, b, 255);
		evas_object_show(offscreen_bg);
	} else { // 1 or 2
		if(!offscreen_img)
			offscreen_img = evas_object_image_add(offscreen_e);
		evas_object_image_size_set(offscreen_img, w, h);
		evas_object_resize(offscreen_img, w, h);

		if(bg_mode == 1) {
			if(bg_wallpaper)
				evas_object_image_file_set(offscreen_img, bg_wallpaper, NULL);
			else
				_E("bg_wallpaper is NULL");
		} else if(bg_mode == 2) {
			if(bg_set)
				evas_object_image_file_set(offscreen_img, bg_set, NULL);
			else
				_E("bg_set is NULL");
		}
		evas_object_image_reload(offscreen_img);
		evas_object_image_filled_set(offscreen_img, EINA_TRUE);
		evas_object_show(offscreen_img);
	}
#endif
	offscreen_box = evas_object_rectangle_add(offscreen_e);
	evas_object_resize(offscreen_box, w, h);
	evas_object_color_set(offscreen_box, 0, 0, 0, 0);
	evas_object_show(offscreen_box);

	ad->win = offscreen_box;

	idle_clock_digital_create_view_main(ad);
	// evas_event_callback_add(e, EVAS_CALLBACK_RENDER_FLUSH_POST, _cb, data);
	_D("create offscreen window\n");

}

static Evas* __live_create_virtual_canvas(int w, int h)
{
	Ecore_Evas *internal_ee;
	Evas *internal_e;

	internal_ee = ecore_evas_buffer_new(w, h);
	if (!internal_ee) {
		_E("Failed to create a new canvas buffer\n");
		return NULL;
	}

	ecore_evas_alpha_set(internal_ee, EINA_TRUE);
	ecore_evas_manual_render_set(internal_ee, EINA_TRUE);

	internal_e = ecore_evas_get(internal_ee);
	if (!internal_e) {
		ecore_evas_free(internal_ee);
		_E("Faield to get Evas object\n");
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
		_E("Failed to get ecore evas\n");
		return -1;
	}

	ecore_evas_manual_render(internal_ee);
	// Get a pointer of a buffer of the virtual canvas
	data = (void*)ecore_evas_buffer_pixels_get(internal_ee);

	if (!data) {
		_E("Failed to get pixel data\n");
		return -1;
	}

	return _flush_data_to_file(e, (char *) data, filename, w, h);
}

static int _flush_data_to_file(Evas *e, char *data, const char *filename, int w, int h)
{
	Evas_Object *output = NULL;

	output = evas_object_image_add(e);
	if (!output) {
		_E("Failed to create an image object\n");
		return -1;
	}

//	_E("%s", filename);
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
		_E("Faield to save a captured image (%s)\n", filename);
		return -1;
	}

	evas_object_del(output);

	// if (access(filename, F_OK) != EPISODE_ERROR_NONE) {
	// 	_E("File %s is not found\n", filename);
	// 	return EPISODE_ERROR;
	// }

	return 0;
}

static Ecore_X_Window
_x11_elm_widget_xwin_get(const Evas_Object *obj)
{
	Evas_Object *top;
	Ecore_X_Window xwin = 0;

	top = elm_widget_top_get(obj);
	if (!top) top = elm_widget_top_get(elm_widget_parent_widget_get(obj));
	if (top) xwin = elm_win_xwindow_get(top);
	if (!xwin) {
		Ecore_Evas *ee;
		Evas *evas = evas_object_evas_get(obj);
		if (!evas) return 0;
		ee = ecore_evas_ecore_evas_get(evas);
		if (!ee) return 0;
		xwin = _elm_ee_xwin_get(ee);
	}
	return xwin;
}

static Eina_Bool
__make_dump(void *data, int win_type, int width, int height, char *file)
{
	appdata *ad = data;
	retvm_if(ad == NULL, false, "Invalid argument: appdata is NULL\n");
	retvm_if(win_type == BUFFER_TYPE_OFFSCREEN, false, "not supported win_type\n");

	Evas_Object *obj = ad->win;

	Ecore_X_Visual visual;
	Ecore_X_Display *display;
	Ecore_X_Screen *scr;
	Ecore_X_Image *img;
	int *src;
	int bpl = 0, rows = 0, bpp = 0;

	Ecore_X_Window xwin = 0;
	Evas *e;
	Ecore_Evas *ee;
	const void *pixel_data;
	int r = 0;
	int g = 0;
	int b = 0;
	// int w, h;

	_D("obj: %x", obj);

	if(win_type == BUFFER_TYPE_WINDOW) {
		// For screen capture
		// xwin = ecore_x_window_root_get(obj);
		xwin = _x11_elm_widget_xwin_get(obj);
		_D("xwin: %x", xwin);
	} else if(win_type == BUFFER_TYPE_MINICONTROL) {
		/********************************************************************************/
		/* workaround..... because of xwin get fail with minicontrol win handle */
		int x=0;
		int y=0;
		int w=0;
		int h=0;
		e = evas_object_evas_get(obj);
		ee = ecore_evas_ecore_evas_get(e);
		ecore_evas_geometry_get(ee, &x, &y, &w, &h);
		_D("##### ~~~~~ minicontrol buffer info. x:%d, y:%d, w:%d, h:%d", x, y, w, h);
		if(w != WIN_SIZE_W || h != WIN_SIZE_H) {
			_E("##### window size was changed by someone!!!");
			evas_object_resize(obj, WIN_SIZE_W, WIN_SIZE_H);
			e = evas_object_evas_get(obj);
			ee = ecore_evas_ecore_evas_get(e);
			ecore_evas_geometry_get(ee, &x, &y, &w, &h);
			_E("##### ##### ~~~~~ minicontrol buffer info. x:%d, y:%d, w:%d, h:%d", x, y, w, h);
		}
		pixel_data = ecore_evas_buffer_pixels_get(ee);
		/********************************************************************************/
	}

#if 1
	_flush_data_to_file(e, pixel_data, DUMP_FILE_PATH_MINICONTROL, 384, WIN_SIZE_H);
	_D("Minicontorl image copied!!!!!!!!!!!!!!!!!!!!!!");

#else
	if(!minicontrol_e)
		minicontrol_e = __live_create_virtual_canvas(width, height);

	if(bg_mode == 0) {
		if(!minicontrol_bg)
			minicontrol_bg = evas_object_rectangle_add(minicontrol_e);
		evas_object_resize(minicontrol_bg, width, width);
		__get_rgb_from_string(&r, &g, &b);
		evas_object_color_set(minicontrol_bg, r, g, b, 255);
		evas_object_show(minicontrol_bg);
	} else {
		if(!minicontrol_img)
			minicontrol_img = evas_object_image_add(minicontrol_e);
		evas_object_image_size_set(minicontrol_img, width, height);
		evas_object_resize(minicontrol_img, width, height);

		if(bg_mode == 1) {
			if(bg_wallpaper) {
				_D("##### bg_wallpaper: %s", bg_wallpaper);
				evas_object_image_file_set(minicontrol_img, bg_wallpaper, NULL);
			} else
				_E("bg_wallpaper is NULL");
		} else if(bg_mode == 2) {
			if(bg_set) {
				evas_object_image_file_set(minicontrol_img, bg_set, NULL);
				_D("##### bg_set: %s", bg_set);
			} else
				_E("bg_set is NULL");
		}
		evas_object_image_reload(minicontrol_img);
		evas_object_image_filled_set(minicontrol_img, EINA_TRUE);
		evas_object_show(minicontrol_img);
	}

	// if(!minicontrol_img_box)
	minicontrol_img_box = evas_object_image_add(minicontrol_e);

	/* for your reference....*/
	// Evas_Object *im = evas_object_image_add(evas_object_evas_get(obj));
	evas_object_size_hint_aspect_set(minicontrol_img_box, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
	evas_object_image_smooth_scale_set(minicontrol_img_box, EINA_FALSE);
	evas_object_image_colorspace_set(minicontrol_img_box, EVAS_COLORSPACE_ARGB8888);
	evas_object_image_size_set(minicontrol_img_box, width, height);
	evas_object_image_filled_set(minicontrol_img_box, EINA_TRUE);
	// evas_object_image_alpha_set(im, EINA_FALSE);
	evas_object_image_alpha_set(minicontrol_img_box, EINA_TRUE);

	if(win_type == BUFFER_TYPE_WINDOW) {
		display = ecore_x_display_get();
		scr = ecore_x_default_screen_get();
		visual = ecore_x_default_visual_get(display, scr);

		img = ecore_x_image_new(width, height, visual, ecore_x_window_depth_get(xwin));
		ecore_x_image_get(img, xwin, 0, 0, 0, 0, width, height);
		src = ecore_x_image_data_get(img, &bpl, &rows, &bpp);
	}

	// Do not use this case now...
	// if (!ecore_x_image_is_argb32_get(img))
	// {
	// 	_D("img is not argb32");
	// 	Ecore_X_Colormap colormap;
	// 	unsigned int *pixels;
	// 	colormap = ecore_x_default_colormap_get(display, scr);
	// 	pixels = evas_object_image_data_get(im, EINA_TRUE);
	// 	ecore_x_image_to_argb_convert(src, bpp, bpl, colormap, visual,
	// 		0, 0, 320, 320,
	// 		pixels, (320 * sizeof(int)), 0, 0);

	// 	evas_object_image_data_copy_set(im, pixels);
	// 	_D("image copied(by converting)!!!!!!!!!!!!!!!!!!!!!!");
	// }

	if(win_type == BUFFER_TYPE_WINDOW)
		evas_object_image_data_copy_set(minicontrol_img_box, src);
	else if(win_type == BUFFER_TYPE_MINICONTROL)
		evas_object_image_data_copy_set(minicontrol_img_box, (void*)pixel_data);

	evas_object_resize(minicontrol_img_box, width, width);
	evas_object_show(minicontrol_img_box);
	live_flush_to_file(minicontrol_e, DUMP_FILE_PATH_MINICONTROL, WIN_SIZE, WIN_SIZE);
	_D("Minicontorl image copied!!!!!!!!!!!!!!!!!!!!!!");

/*
	Eina_Bool r = evas_object_image_save(im, file, NULL, NULL);
	if(r)
		_D("save success");
	else
		_D("save fail");
*/

	if(minicontrol_img_box) {
		evas_object_del(minicontrol_img_box);
		minicontrol_img_box = NULL;
	}

	if(win_type == BUFFER_TYPE_WINDOW)
		ecore_x_image_free(img);
#endif

	// evas_object_geometry_get(im, NULL, NULL, &w, &h);
	// evas_object_image_data_update_add(im, 0, 0, w, h);
	return EINA_TRUE;
}

static bool __create_window(void *data, int mode)
{
	appdata *ad = data;
	retvm_if(ad == NULL, false, "Invalid argument: appdata is NULL\n");

	/* initialize variables */
	ad->win_w = 0;
	ad->win_h = 0;

	Evas_Object *win = NULL;

	/* create main window */
	if(ad->win == NULL) {
		if(mode == BUFFER_TYPE_OFFSCREEN) {
			ad->win_type = BUFFER_TYPE_OFFSCREEN;
			__update_clock_to_offscreen(data, WIN_SIZE_W, WIN_SIZE_H);

			return true;
		}
		win = idle_clock_digital_window_create(PACKAGE);
		retvm_if(win == NULL, -1, "Failed add window\n");
		evas_object_resize(win, WIN_SIZE_W, WIN_SIZE_H);
		evas_object_move(win, 0, 0);
		ad->win = win;
		ad->win_type = BUFFER_TYPE_MINICONTROL;
		idle_clock_digital_create_view_main(ad);
		_D("create window\n");
	}
	return true;
}

static bool __idle_clock_digital_create(void *data)
{
	_D("%s", __func__);
	return true;
}

static void __idle_clock_digital_terminate(void *data)
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

	if(close_timer != NULL) {
		ecore_timer_del(close_timer);
		close_timer = NULL;
	}

	idle_clock_destroy_view_main(ad);

}

static void __idle_clock_digital_pause(void *data)
{
	_D("%s", __func__);
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
}

static void __idle_clock_digital_resume(void *data)
{
	_D("%s", __func__);
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
}

static void __send_reply(void *data)
{
	_D("%s", __func__);
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	/* send reply */
	app_control_h reply;
	int ret = app_control_create(&reply);
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

static void __draw_onscreen(void *data)
{
	_D();
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	__make_dump(data, ad->win_type, WIN_SIZE_W, WIN_SIZE_H, DUMP_FILE_PATH_MINICONTROL);
	__send_reply(data);

	if(_get_pm_state() == VCONFKEY_PM_STATE_LCDOFF)
		hide_clock(ad);

	drawing_state = 0;
	drawing_timer = NULL;
}

static Eina_Bool __idler_drawing_onscreen_cb(void *data)
{
	_D();
	appdata *ad = data;
	retvm_if(ad == NULL, ECORE_CALLBACK_RENEW, "Invalid argument: appdata is NULL\n");

	__draw_onscreen(ad);

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __drawing_timer_onscreen_cb(void *data)
{
	appdata *ad = data;
	retvm_if(ad == NULL, ECORE_CALLBACK_RENEW, "Invalid argument: appdata is NULL\n");

	if(!ad->is_show) {
		show_clock(ad);
		ecore_idler_add(__idler_drawing_onscreen_cb, ad);
		return ECORE_CALLBACK_CANCEL;
	}

	__draw_onscreen(ad);

	return ECORE_CALLBACK_CANCEL;
}

static void __draw_offscreen(void *data)
{
	_D();
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");

	live_flush_to_file(ad->e_offscreen, DUMP_FILE_PATH_OFFSCREEN, WIN_SIZE_W, WIN_SIZE_H);

	__send_reply(data);

	if(_get_pm_state() == VCONFKEY_PM_STATE_LCDOFF)
		hide_clock(ad);

	drawing_state = 0;
	drawing_timer = NULL;

	_D("win type = %d and window = %d", ad->win_type, ad->win);
	if(ad->win_type == BUFFER_TYPE_OFFSCREEN)
	{
		_D("offscreen capture completed. app will be closed");
		elm_exit();
	}
}

static Eina_Bool __idler_drawing_offscreen_cb(void *data)
{
	_D();
	appdata *ad = data;
	retvm_if(ad == NULL, ECORE_CALLBACK_RENEW, "Invalid argument: appdata is NULL\n");

	__draw_offscreen(ad);

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __drawing_timer_cb(void *data)
{
	_D();
	appdata *ad = data;
	retvm_if(ad == NULL, ECORE_CALLBACK_RENEW, "Invalid argument: appdata is NULL\n");

	if(!ad->is_show) {
		show_clock(ad);
		ecore_idler_add(__idler_drawing_offscreen_cb, ad);
		return ECORE_CALLBACK_CANCEL;
	}

	__draw_offscreen(ad);

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __close_timer_cb(void *data)
{
	_D();
	elm_exit();
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __sync_timer_cb(void *data)
{
	_D();
	appdata *ad = data;
	retvm_if(ad == NULL, ECORE_CALLBACK_RENEW, "Invalid argument: appdata is NULL\n");

	if(drawing_state == 0) {
		__remove_preview_resource(data);
		__create_window(data, BUFFER_TYPE_MINICONTROL);
		sync_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}

	return ECORE_CALLBACK_RENEW;
}

static void __idle_clock_digital_app_control(app_control_h app_control, void *data)
{
	_D();
	appdata *ad = data;
	retm_if(ad == NULL, "Invalid argument: appdata is NULL\n");
	char *op = NULL;
	char *result_data = NULL;

	app_control_get_operation(app_control, &op);

	if(op != NULL) {
		_D("operation:%s", op);
		if(strcmp(op, "http://tizen.org/appcontrol/operation/remote_settings") == 0) {
			app_control_get_extra_data(app_control, "http://tizen.org/appcontrol/data/result_xml", &result_data);
			if(result_data) {
				_D("Result:%s", result_data);
				idle_clock_digital_parse_result_data(result_data);
				idle_clock_digital_set_result_data(ad);
				idle_clock_digital_update_view(ad);
				free(result_data);

				if (ad->win_type == BUFFER_TYPE_MINICONTROL && ad->win) {
					display_change_state(LCD_NORMAL);
				} else {
					_D("app will be closed");
					if(close_timer != NULL) {
						ecore_timer_del(close_timer);
						close_timer = NULL;
					}
					close_timer = ecore_timer_add(3.0, __close_timer_cb, NULL);
				}
			}
		}
		else if(strcmp(op, "http://tizen.org/appcontrol/operation/main") == 0) {
			if(close_timer != NULL) {
				ecore_timer_del(close_timer);
				close_timer = NULL;
			}
			if(drawing_state) {
				if (sync_timer != NULL) {
					ecore_timer_del(sync_timer);
					sync_timer = NULL;
				}
				sync_timer = ecore_timer_add(0.1, __sync_timer_cb, data);
			} else {
				/* New case: offscreen capture -> operation/main */
				__remove_preview_resource(data);
				__create_window(data, BUFFER_TYPE_MINICONTROL); /* create window if not mini app setting called */
			}
		}
		else if(strcmp(op, "http://tizen.org/appcontrol/operation/clock/capture") == 0) {
			app_control_clone(&ad->app_control, app_control);

			if(close_timer != NULL) {
				ecore_timer_del(close_timer);
				close_timer = NULL;
			}

			__update_bg_info();
			__create_window(data, BUFFER_TYPE_OFFSCREEN);

			if(ad->win_type == BUFFER_TYPE_OFFSCREEN) {
				_D("offscreen capture");
				drawing_state = 1;

				if (drawing_timer != NULL) {
					ecore_timer_del(drawing_timer);
					drawing_timer = NULL;
				}
				__set_info_time(data);
				if(_get_pm_state() == VCONFKEY_PM_STATE_LCDOFF)
					show_clock(ad);
				drawing_timer = ecore_timer_add(0.15, __drawing_timer_cb, data);

#ifdef PROFILE
				Stop(&sw);
				_D("elapsed time [%ld]mm\n", GetElapsedMM(&sw));
#endif
			} else {
				_D("minicontrol capture");
				drawing_state = 2;
				if (drawing_timer != NULL) {
					ecore_timer_del(drawing_timer);
					drawing_timer = NULL;
				}
				__set_info_time(data);
				if(_get_pm_state() == VCONFKEY_PM_STATE_LCDOFF)
					show_clock(ad);
				drawing_timer = ecore_timer_add(0.15, __drawing_timer_onscreen_cb, data);
			}
		}
		else {
			_E("Unknown operation");
		}
		free(op);
	}
}

int main(int argc, char *argv[])
{
	appdata ad;

	app_event_callback_s event_callback;

	event_callback.create = __idle_clock_digital_create;
	event_callback.terminate = __idle_clock_digital_terminate;
	event_callback.pause = __idle_clock_digital_pause;
	event_callback.resume = __idle_clock_digital_resume;
	event_callback.app_control = __idle_clock_digital_app_control;
	event_callback.low_memory = NULL;
	event_callback.low_battery = NULL;
	event_callback.device_orientation = NULL;
	event_callback.language_changed = NULL;
	event_callback.region_format_changed = NULL;

	memset(&ad, 0x0, sizeof(appdata));

	return app_efl_main(&argc, &argv, &event_callback, &ad);
}

