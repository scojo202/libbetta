/* This is -*- C -*- */
/* vim: set sw=2: */
/* $Id: guppi-color-palette.c,v 1.7 2002/01/08 06:31:07 trow Exp $ */

/*
 * guppi-color-palette.c
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

#include "y-color-map.h"

#include <math.h>
#include <string.h>

/**
 * SECTION: y-color-map
 * @short_description: Object for holding a set of colors.
 *
 *
 *
 */

static GObjectClass *parent_class = NULL;

struct _YColorMap {
  GObject parent;

  gchar *meta;
  gint N;
  guint32 *nodes;
  gint offset, intensity, alpha;
  gboolean flip, own_nodes;
};

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint y_color_map_signals[LAST_SIGNAL] = { 0 };

typedef enum {
  PALETTE_CUSTOM,
  PALETTE_MONOCHROME,
  PALETTE_STOCK,
  PALETTE_ALIEN_STOCK,
  PALETTE_TRANSITION,
  PALETTE_FADE,
  PALETTE_FIRE,
  PALETTE_ICE,
  PALETTE_THERMAL,
  PALETTE_SPECTRUM,
  PALETTE_INVALID,
  PALETTE_LAST
} PaletteStyle;

typedef struct {
  PaletteStyle code;
  const gchar *meta;
  gboolean parameterless;
} PaletteInfo;

static const PaletteInfo palette_info[PALETTE_LAST] = {

  { PALETTE_CUSTOM,      "custom",      FALSE },
  { PALETTE_MONOCHROME,  "monochrome",  FALSE },
  { PALETTE_STOCK,       "stock",       TRUE  },
  { PALETTE_ALIEN_STOCK, "alien_stock", TRUE  },
  { PALETTE_TRANSITION,  "transition",  FALSE },
  { PALETTE_FADE,        "fade",        FALSE },
  { PALETTE_FIRE,        "fire",        TRUE  },
  { PALETTE_ICE,         "ice",         TRUE  },
  { PALETTE_THERMAL,     "thermal",     TRUE  },
  { PALETTE_SPECTRUM,    "spectrum",    TRUE  },
  { PALETTE_INVALID,     NULL,          FALSE }
};

/**
 * y_color_map_new:
 *
 * Create a new #YColorMap and set it to the default "stock" palette.
 *
 * Returns: the new color map.
 **/
YColorMap *
y_color_map_new (void)
{
  YColorMap *pal = Y_COLOR_MAP (g_object_new (y_color_map_get_type (), NULL));
  y_color_map_set_stock (pal);

  return pal;
}

/**
 * y_color_map_copy:
 * @pal: a #YColorMap
 *
 * Create a copy of @pal, an existing #YColorMap.
 *
 * Returns: (transfer full): the new color map.
 **/
YColorMap *
y_color_map_copy (YColorMap *pal)
{
  YColorMap *new_pal = Y_COLOR_MAP (g_object_new (y_color_map_get_type (),NULL));

  new_pal->meta      = g_strdup (pal->meta);
  new_pal->N         = pal->N;
  new_pal->offset    = pal->offset;
  new_pal->intensity = pal->intensity;
  new_pal->alpha     = pal->alpha;
  new_pal->flip      = pal->flip;
  new_pal->own_nodes = pal->own_nodes;

  if (new_pal->own_nodes) {
    new_pal->nodes = g_new (guint32, pal->N);
    memcpy (new_pal->nodes, pal->nodes, sizeof (guint32) * pal->N);
  } else {
    new_pal->nodes = pal->nodes;
  }

  return new_pal;
}

/**
 * y_color_map_size:
 * @pal: a #YColorMap
 *
 * Get the number of colors in @pal.
 *
 * Returns: the number of colors.
 **/
gint
y_color_map_size (YColorMap *pal)
{
  g_return_val_if_fail (Y_IS_COLOR_MAP (pal), -1);

  return pal->N;
}

/**
 * y_color_map_get:
 * @pal: a #YColorMap
 * @i: an integer
 *
 * Get the ith color in @pal.
 *
 * Returns: the color
 **/
guint32
y_color_map_get (YColorMap *pal, gint i)
{
  guint32 c, r, g, b, a;

  g_return_val_if_fail (Y_IS_COLOR_MAP (pal), 0);

  if (pal->N < 1) {
    return 0;
  }

  if (pal->N > 1) {
    i = (i + pal->offset) % pal->N;
    if (i < 0)
      i += pal->N;

    if (pal->flip)
      i = pal->N - 1 - i;
  } else {
    i = 0;
  }

  c = pal->nodes[i];

  if (c == 0 || pal->intensity == 0 || pal->alpha == 0)
    return 0;

  if (pal->intensity == 255 || pal->alpha == 255)
    return c;

  UINT_TO_RGBA (c, &r, &g, &b, &a);

  r = (r * pal->intensity + 0x80) >> 8;
  g = (g * pal->intensity + 0x80) >> 8;
  b = (b * pal->intensity + 0x80) >> 8;
  a = (a * pal->alpha     + 0x80) >> 8;

  return RGBA_TO_UINT (r, g, b, a);
}

/**
 * y_color_map_interpolate:
 * @pal: a #YColorMap
 * @t: a number
 *
 * Interpolate between colors floor(t) and ceil(t) in the map.
 *
 * Returns: the color
 **/
guint32
y_color_map_interpolate (YColorMap *pal, double t)
{
  guint32 c1, c2;
  gint i, f1, f2;
  gint r1, g1, b1, a1;
  gint r2, g2, b2, a2;

  g_return_val_if_fail (Y_IS_COLOR_MAP (pal), 0);

  if (pal->N < 1) {
    return 0;
  } else if (pal->N == 1) {
    return y_color_map_get (pal, 0);
  }

  i = (gint) floor (t);
  f2 = (gint) floor (256 * (t - i));
  f1 = 256 - f2;

  c1 = y_color_map_get (pal, i);
  c2 = y_color_map_get (pal, i+1);

  if (c1 == c2 || f2 == 0)
    return c1;

  UINT_TO_RGBA (c1, &r1, &g1, &b1, &a1);
  UINT_TO_RGBA (c2, &r2, &g2, &b2, &a2);

  if (r1 != r2)
    r1 = (f1 * r1 + f2 * r2) >> 8;

  if (g1 != g2)
    g1 = (f1 * g1 + f2 * g2) >> 8;

  if (b1 != b2)
    b1 = (f1 * b1 + f2 * b2) >> 8;

  if (a1 != a2)
    a1 = (f1 * a1 + f2 * a2) >> 8;

  return RGBA_TO_UINT (r1, g1, b1, a1);
}

/**
 * y_color_map_get_map:
 * @pal: a #YColorMap
 * @t: a number between 0.0 and 1.0
 *
 * Interpolate between colors floor(t) and ceil(t) in the map.
 *
 * Returns: the color
 **/
guint32  y_color_map_get_map (YColorMap *pal, double t)
{
  g_return_val_if_fail (Y_IS_COLOR_MAP (pal),0);
  if(t<=0.0)
    return y_color_map_get(pal,0);
  if(t>=1.0)
    return y_color_map_get(pal,pal->N-1);
  return y_color_map_interpolate(pal,(pal->N-1)*t);
}

void
y_color_map_set (YColorMap *pal, gint i, guint32 col)
{
  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  if (pal->N < 1)
    return;

  /* Map index onto palette */
  if (pal->N > 1) {
    i = (i + pal->offset) % pal->N;
    if (i < 0)
      i += pal->N;

    if (pal->flip)
      i = pal->N - 1 - i;
  } else {
    i = 0;
  }

  if (pal->nodes[i] == col)
    return;

  if (! pal->own_nodes) {
    guint32 *nodes = pal->nodes;
    pal->nodes = g_new (guint32, pal->N);
    memcpy (pal->nodes, nodes, sizeof (guint32) * pal->N);
    pal->own_nodes = TRUE;
  }

  pal->nodes[i] = col;
  g_free (pal->meta);
  pal->meta = g_strdup (palette_info[PALETTE_CUSTOM].meta);

  g_signal_emit (G_OBJECT (pal), y_color_map_signals[CHANGED], 0);
}

gint
y_color_map_get_offset (YColorMap *pal)
{
  g_return_val_if_fail (Y_IS_COLOR_MAP (pal), 0);
  return pal->offset;
}

void
y_color_map_set_offset (YColorMap *pal, gint offset)
{
  gint shift;

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  shift = offset - pal->offset;
  pal->offset = offset;

  if (pal->N > 1) {
    shift = shift % pal->N;
    if (shift < 0)
      shift += pal->N;
    if (shift != 0)
      g_signal_emit (G_OBJECT (pal), y_color_map_signals[CHANGED], 0);
  }
}

gint
y_color_map_get_alpha (YColorMap *pal)
{
  g_return_val_if_fail (Y_IS_COLOR_MAP (pal), -1);
  return pal->alpha;
}

void
y_color_map_set_alpha (YColorMap *pal, gint alpha)
{
  g_return_if_fail (Y_IS_COLOR_MAP (pal));
  g_return_if_fail (0 <= alpha && alpha <= 255);

  if (pal->alpha != alpha) {
    pal->alpha = alpha;
    g_signal_emit (G_OBJECT (pal), y_color_map_signals[CHANGED], 0);
  }
}

gint
y_color_map_get_intensity (YColorMap *pal)
{
  g_return_val_if_fail (Y_IS_COLOR_MAP (pal), -1);
  return pal->intensity;
}

void
y_color_map_set_intensity (YColorMap *pal, gint intensity)
{
  g_return_if_fail (Y_IS_COLOR_MAP (pal));
  g_return_if_fail (0 <= intensity && intensity <= 255);
  if (pal->intensity != intensity) {
    pal->intensity = intensity;
    g_signal_emit (G_OBJECT (pal), y_color_map_signals[CHANGED], 0);
  }
}

gboolean
y_color_map_get_flipped (YColorMap *pal)
{
  g_return_val_if_fail (Y_IS_COLOR_MAP (pal), FALSE);
  return pal->flip;
}

void
y_color_map_set_flipped (YColorMap *pal, gboolean f)
{
  g_return_if_fail (Y_IS_COLOR_MAP (pal));
  if (pal->flip != f) {
    pal->flip = f;
    g_signal_emit (G_OBJECT (pal), y_color_map_signals[CHANGED], 0);
  }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_color_map_set_raw (YColorMap *pal, const gchar *meta, guint32 *nodes, gint N, gboolean owned)
{
  g_return_if_fail (Y_IS_COLOR_MAP (pal));
  g_return_if_fail (nodes != NULL);
  g_return_if_fail (N > 0);

  if (pal->meta && !strcmp (meta, pal->meta) && pal->nodes == nodes && pal->N == N)
    return;

  if (pal->own_nodes)
    g_free (pal->nodes);

  g_free (pal->meta);
  pal->meta = g_strdup (meta);

  pal->nodes = nodes;
  pal->N = N;
  pal->own_nodes = owned;

  g_signal_emit (G_OBJECT (pal), y_color_map_signals[CHANGED], 0);
}

void
y_color_map_set_stock (YColorMap *pal)
{
  static guint32 stock_colors[] = {
    0xff3000ff, 0x80ff00ff, 0x00ffcfff, 0x2000ffff,
    0xff008fff, 0xffbf00ff, 0x00ff10ff, 0x009fffff,
    0xaf00ffff, 0xff0000ff, 0xafff00ff, 0x00ff9fff,
    0x0010ffff, 0xff00bfff, 0xff8f00ff, 0x20ff00ff,
    0x00cfffff, 0x8000ffff, 0xff0030ff, 0xdfff00ff,
    0x00ff70ff, 0x0040ffff, 0xff00efff, 0xff6000ff,
    0x50ff00ff, 0x00ffffff, 0x5000ffff, 0xff0060ff,
    0xffef00ff, 0x00ff40ff, 0x0070ffff, 0xdf00ffff
  };

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  y_color_map_set_raw (pal,
			       palette_info[PALETTE_STOCK].meta,
			       stock_colors,
			       sizeof (stock_colors) / sizeof (guint32),
			       FALSE);
}

void
y_color_map_set_alien_stock (YColorMap *pal)
{
  static guint32 alien_stock_colors[] = {
    0x9c9cffff, 0x9c3163ff, 0xffffceff, 0xceffffff,
    0x630063ff, 0xff8484ff, 0x0063ceff, 0xceceffff,
    0x000084ff, 0xff00ffff, 0xffff00ff, 0x00ffffff,
    0x840084ff, 0x840000ff, 0x008484ff, 0x0000ffff,
    0x00ceffff, 0xceffffff, 0xceffceff, 0xffff9cff,
    0x9cceffff, 0xff9cceff, 0xce9cffff, 0xffce9cff,
    0x3163ffff, 0x31ceceff, 0x9cce00ff, 0xffce00ff,
    0xff9c00ff, 0xff6300ff, 0x63639cff, 0x949494ff,
    0x003163ff, 0x319c63ff, 0x003100ff, 0x313100ff,
    0x9c3100ff, 0x9c3163ff, 0x31319cff, 0x313131ff,
    0xffffffff, 0xff0000ff, 0x00ff00ff, 0x0000ffff,
    0xffff00ff, 0xff00ffff, 0x00ffffff, 0x840000ff,
    0x008400ff, 0x000084ff, 0x848400ff, 0x840084ff,
    0x008484ff, 0xc6c6c6ff, 0x848484ff
  };

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  y_color_map_set_raw (pal,
			       palette_info[PALETTE_ALIEN_STOCK].meta,
			       alien_stock_colors,
			       sizeof (alien_stock_colors) / sizeof (guint32),
			       FALSE);
}

/**
 * y_color_map_set_transition:
 * @pal: a #YColorMap
 * @c1: a color
 * @c2: another color
 *
 * Set the color map palette to have two colors.
 **/
void
y_color_map_set_transition (YColorMap *pal, guint32 c1, guint32 c2)
{
  guint32 *nodes;

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  nodes = g_new (guint32, 2);
  nodes[0] = c1;
  nodes[1] = c2;

  y_color_map_set_raw (pal, palette_info[PALETTE_TRANSITION].meta, nodes, 2, TRUE);
}

void
y_color_map_set_fade (YColorMap *pal, guint32 c)
{
  guint32 *nodes;

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  nodes = g_new (guint32, 2);
  nodes[0] = 0;
  nodes[1] = c;

  y_color_map_set_raw (pal, palette_info[PALETTE_FADE].meta, nodes, 2, TRUE);
}

void
y_color_map_set_fire (YColorMap *pal)
{
  static guint32 fire_colors[] = {
    0xff000000, 0xff0000ff, 0xffff00ff
  };

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  y_color_map_set_raw (pal,
			       palette_info[PALETTE_FIRE].meta,
			       fire_colors,
			       sizeof (fire_colors) / sizeof (guint32),
			       FALSE);

}

void
y_color_map_set_ice (YColorMap *pal)
{
  static guint32 ice_colors[] = {
    0xff000000, 0xff0000ff, 0xffff00ff
  };

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  y_color_map_set_raw (pal,
			       palette_info[PALETTE_ICE].meta,
			       ice_colors,
			       sizeof (ice_colors) / sizeof (guint32),
			       FALSE);

}

void
y_color_map_set_thermal (YColorMap *pal)
{
  static guint32 thermal_colors[] = {
    0xffffff00, 0xff00ff00, 0xff008080, 0xff0000ff, 0xffff00ff
  };

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  y_color_map_set_raw (pal,
			       palette_info[PALETTE_THERMAL].meta,
			       thermal_colors,
			       sizeof (thermal_colors) / sizeof (guint32),
			       FALSE);
}


void
y_color_map_set_spectrum (YColorMap *pal)
{
  static guint32 spectrum_colors[] = {
    0xff0000ff, 0xff8000ff, 0xffff00ff, 0x33cc33ff, 0x0080ffff, 0xff80ffff
  };

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  y_color_map_set_raw (pal,
			       palette_info[PALETTE_SPECTRUM].meta,
			       spectrum_colors,
			       sizeof (spectrum_colors) / sizeof (guint32),
			       FALSE);
}

/**
 * y_color_map_set_monochrome:
 * @pal: a #YColorMap
 * @c: a color
 *
 * Set the color map palette to have only one color.
 **/
void
y_color_map_set_monochrome (YColorMap *pal, guint32 c)
{
  guint32 *cc;

  g_return_if_fail (Y_IS_COLOR_MAP (pal));

  cc = g_new (guint32, 1);
  *cc = c;
  y_color_map_set_raw (pal,
			       palette_info[PALETTE_MONOCHROME].meta,
			       cc,
			       1,
			       TRUE);
}

/**
 * y_color_map_set_custom:
 * @pal: a #YColorMap
 * @N: the number of colors
 * @color: an array of colors
 *
 * Set the color map palette from an array of colors
 **/
void
y_color_map_set_custom (YColorMap *pal, gint N, guint32 *color)
{
  guint32 *color_cpy;
  gint i;

  g_return_if_fail (Y_IS_COLOR_MAP (pal));
  g_return_if_fail (N > 0);

  /* If color is passed in as NULL, set the palette to all black. */

  color_cpy = g_new (guint32, N);
  for (i = 0; i < N; ++i)
    color_cpy[i] = color ? color[i] : RGBA_BLACK;

  y_color_map_set_raw (pal,
			       N > 1 ? palette_info[PALETTE_CUSTOM].meta : palette_info[PALETTE_MONOCHROME].meta,
			       color_cpy, N, TRUE);
}

void
y_color_map_set_vcustom (YColorMap *pal, gint N, guint32 first_color, ...)
{
  guint32 *color;
  gint i;
  va_list args;

  g_return_if_fail (Y_IS_COLOR_MAP (pal));
  g_return_if_fail (N > 0);

  color = g_new (guint32, N);

  color[0] = first_color;

  i = 1;
  va_start (args, first_color);
  while (i < N) {
    color[i] = (guint32) va_arg (args, gint);
    ++i;
  }
  va_end (args);

  y_color_map_set_raw (pal,
			       N > 1 ? palette_info[PALETTE_CUSTOM].meta : palette_info[PALETTE_MONOCHROME].meta,
			       color, N, TRUE);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_color_map_finalize (GObject *obj)
{
  YColorMap *x = Y_COLOR_MAP(obj);

  if (x->own_nodes) {
    g_free (x->nodes);
  }
  g_free (x->meta);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

G_DEFINE_TYPE (YColorMap, y_color_map, G_TYPE_OBJECT);

static void
y_color_map_class_init (YColorMapClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_color_map_finalize;

  y_color_map_signals[CHANGED] =
    g_signal_new ("changed",
                    G_TYPE_FROM_CLASS(klass),
                    G_SIGNAL_RUN_FIRST,
                    0,NULL,NULL,
                    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
y_color_map_init (YColorMap *pal)
{
  pal->meta      = g_strdup ("custom");
  pal->alpha     = 0xff;
  pal->intensity = 0xff;
}
