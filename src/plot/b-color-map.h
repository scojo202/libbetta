/*
 * b-color-palette.h
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

#pragma once

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BColorMap,b_color_map,B,COLOR_MAP,GObject)

#define B_TYPE_COLOR_MAP (b_color_map_get_type())

BColorMap *b_color_map_new  (void);
BColorMap *b_color_map_copy (BColorMap *pal);

gint     b_color_map_size            (BColorMap *pal);
guint32  b_color_map_get             (BColorMap *pal, gint i);
guint32  b_color_map_interpolate     (BColorMap *pal, double t);
guint32  b_color_map_get_map         (BColorMap *pal, double t);

void     b_color_map_set             (BColorMap *pal, gint i, guint32 col);

gint     b_color_map_get_offset      (BColorMap *pal);
void     b_color_map_set_offset      (BColorMap *pal, gint offset);

gint     b_color_map_get_alpha       (BColorMap *pal);
void     b_color_map_set_alpha       (BColorMap *pal, gint alpha);

gint     b_color_map_get_intensity   (BColorMap *pal);
void     b_color_map_set_intensity   (BColorMap *pal, gint intensity);

gboolean b_color_map_get_flipped     (BColorMap *pal);
void     b_color_map_set_flipped     (BColorMap *pal, gboolean f);

void     b_color_map_set_stock       (BColorMap *pal);
void     b_color_map_set_alien_stock (BColorMap *pal);
void     b_color_map_set_transition  (BColorMap *pal, guint32 c1, guint32 c2);
void     b_color_map_set_fade        (BColorMap *pal, guint32 c);
void     b_color_map_set_fire        (BColorMap *pal);
void     b_color_map_set_ice         (BColorMap *pal);
void     b_color_map_set_thermal     (BColorMap *pal);
void     b_color_map_set_jet         (BColorMap *pal);
void     b_color_map_set_seismic (BColorMap *pal);
void     b_color_map_set_spectrum    (BColorMap *pal);
void     b_color_map_set_monochrome  (BColorMap *pal, guint32 c);

void     b_color_map_set_custom      (BColorMap *pal, gint N, guint32 *color);
void     b_color_map_set_vcustom     (BColorMap *pal, gint N, guint32 first_color, ...);

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
