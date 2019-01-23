/*
 * guppi-color-palette.h
 *
 * Copyright (C) 2001 The Free Software Foundation
 *
 * Developed by Jon Trowbridge <trow@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef _INC_Y_COLOR_MAP_H
#define _INC_Y_COLOR_MAP_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(YColorMap,y_color_map,Y,COLOR_MAP,GObject)

#define Y_TYPE_COLOR_MAP (y_color_map_get_type())

YColorMap *y_color_map_new  (void);
YColorMap *y_color_map_copy (YColorMap *pal);

gint     y_color_map_size            (YColorMap *pal);
guint32  y_color_map_get             (YColorMap *pal, gint i);
guint32  y_color_map_interpolate     (YColorMap *pal, double t);
guint32  y_color_map_get_map         (YColorMap *pal, double t);

void     y_color_map_set             (YColorMap *pal, gint i, guint32 c);

gint     y_color_map_get_offset      (YColorMap *pal);
void     y_color_map_set_offset      (YColorMap *pal, gint offset);

gint     y_color_map_get_alpha       (YColorMap *pal);
void     y_color_map_set_alpha       (YColorMap *pal, gint alpha);

gint     y_color_map_get_intensity   (YColorMap *pal);
void     y_color_map_set_intensity   (YColorMap *pal, gint intensity);

gboolean y_color_map_get_flipped     (YColorMap *pal);
void     y_color_map_set_flipped     (YColorMap *pal, gboolean f);

void     y_color_map_set_stock       (YColorMap *pal);
void     y_color_map_set_alien_stock (YColorMap *pal);
void     y_color_map_set_transition  (YColorMap *pal, guint32 c1, guint32 c2);
void     y_color_map_set_fade        (YColorMap *pal, guint32 c);
void     y_color_map_set_fire        (YColorMap *pal);
void     y_color_map_set_ice         (YColorMap *pal);
void     y_color_map_set_thermal     (YColorMap *pal);
void     y_color_map_set_spectrum    (YColorMap *pal);
void     y_color_map_set_monochrome  (YColorMap *pal, guint32 c);

void     y_color_map_set_custom      (YColorMap *pal, gint N, guint32 *color);
void     y_color_map_set_vcustom     (YColorMap *pal, gint N, guint32 first_color, ...);

#define RGB_TO_UINT(r,g,b) ((((guint)(r)))|(((guint)(g))<<8)|((guint)(b))<<16)
#define RGB_TO_RGBA(x,a) ((x) | ((((guint)a) & 0xff000000)))
#define RGBA_TO_UINT(r,g,b,a) RGB_TO_RGBA(RGB_TO_UINT(r,g,b), a)
#define UINT_TO_RGB(u,r,g,b) \
{ (*(r)) = ((u))&0xff; (*(g)) = ((u)>>8)&0xff; (*(b)) = ((u)>>16)&0xff; }
#define UINT_TO_RGBA(u,r,g,b,a) \
{ UINT_TO_RGB(((u)),r,g,b); (*(a)) = (u)&0xff000000; }
#define RGB_WHITE  RGB_TO_UINT(0xff, 0xff, 0xff)
#define RGB_BLACK  RGB_TO_UINT(0x00, 0x00, 0x00)
#define RGB_RED    RGB_TO_UINT(0xff, 0x00, 0x00)
#define RGB_GREEN  RGB_TO_UINT(0x00, 0xff, 0x00)
#define RGB_BLUE   RGB_TO_UINT(0x00, 0x00, 0xff)
#define RGB_YELLOW RGB_TO_UINT(0xff, 0xff, 0x00)
#define RGB_VIOLET RGB_TO_UINT(0xff, 0x00, 0xff)
#define RGB_CYAN   RGB_TO_UINT(0x00, 0xff, 0xff)
#define RGBA_WHITE  RGB_TO_RGBA(RGB_WHITE, 0xff)
#define RGBA_BLACK  RGB_TO_RGBA(RGB_BLACK, 0xff)
#define RGBA_RED    RGB_TO_RGBA(RGB_RED, 0xff)
#define RGBA_GREEN  RGB_TO_RGBA(RGB_GREEN, 0xff)
#define RGBA_BLUE   RGB_TO_RGBA(RGB_BLUE, 0xff)
#define RGBA_YELLOW RGB_TO_RGBA(RGB_YELLOW, 0xff)
#define RGBA_VIOLET RGB_TO_RGBA(RGB_VIOLET, 0xff)
#define RGBA_CYAN   RGB_TO_RGBA(RGB_CYAN, 0xff)

G_END_DECLS

#endif /* _INC_Y_COLOR_PALETTE_H */
