/*
 * b-color-palette.c
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

#include "b-color-map.h"

#include <math.h>
#include <string.h>

/**
 * SECTION: b-color-map
 * @short_description: Object for holding a set of colors.
 *
 *
 *
 */

static GObjectClass *parent_class = NULL;

struct _BColorMap {
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

static guint b_color_map_signals[LAST_SIGNAL] = { 0 };

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
  PALETTE_JET,
  PALETTE_SEISMIC,
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
  { PALETTE_JET,         "jet",         TRUE  },
  { PALETTE_SEISMIC,     "seismic",     TRUE  },
  { PALETTE_INVALID,     NULL,          FALSE }
};

/**
 * b_color_map_new:
 *
 * Create a new #BColorMap and set it to the default "stock" palette.
 *
 * Returns: the new color map.
 **/
BColorMap *
b_color_map_new (void)
{
  BColorMap *pal = B_COLOR_MAP (g_object_new (b_color_map_get_type (), NULL));
  b_color_map_set_stock (pal);

  return pal;
}

/**
 * b_color_map_copy:
 * @pal: a #BColorMap
 *
 * Create a copy of @pal, an existing #BColorMap.
 *
 * Returns: (transfer full): the new color map.
 **/
BColorMap *
b_color_map_copy (BColorMap *pal)
{
  BColorMap *new_pal = B_COLOR_MAP (g_object_new (b_color_map_get_type (),NULL));

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
 * b_color_map_size:
 * @pal: a #BColorMap
 *
 * Get the number of colors in @pal.
 *
 * Returns: the number of colors.
 **/
gint
b_color_map_size (BColorMap *pal)
{
  g_return_val_if_fail (B_IS_COLOR_MAP (pal), -1);

  return pal->N;
}

/**
 * b_color_map_get:
 * @pal: a #BColorMap
 * @i: an integer
 *
 * Get the ith color in @pal.
 *
 * Returns: the color
 **/
guint32
b_color_map_get (BColorMap *pal, gint i)
{
  guint32 c, r, g, b, a;

  g_return_val_if_fail (B_IS_COLOR_MAP (pal), 0);

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
 * b_color_map_interpolate:
 * @pal: a #BColorMap
 * @t: a number
 *
 * Interpolate between colors floor(t) and ceil(t) in the map.
 *
 * Returns: the color
 **/
guint32
b_color_map_interpolate (BColorMap *pal, double t)
{
  guint32 c1, c2;
  gint i, f1, f2;
  gint r1, g1, b1, a1;
  gint r2, g2, b2, a2;

  g_return_val_if_fail (B_IS_COLOR_MAP (pal), 0);

  if (pal->N < 1) {
    return 0;
  } else if (pal->N == 1) {
    return b_color_map_get (pal, 0);
  }

  i = (gint) floor (t);
  f2 = (gint) floor (256 * (t - i));
  f1 = 256 - f2;

  c1 = b_color_map_get (pal, i);
  c2 = b_color_map_get (pal, i+1);

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
 * b_color_map_get_map:
 * @pal: a #BColorMap
 * @t: a number between 0.0 and 1.0
 *
 * Interpolate between colors floor(t) and ceil(t) in the map.
 *
 * Returns: the color
 **/
guint32  b_color_map_get_map (BColorMap *pal, double t)
{
  g_return_val_if_fail (B_IS_COLOR_MAP (pal),0);
  if(t<=0.0)
    return b_color_map_get(pal,0);
  if(t>=1.0)
    return b_color_map_get(pal,pal->N-1);
  return b_color_map_interpolate(pal,(pal->N-1)*t);
}

/**
 * b_color_map_set:
 * @pal: a #BColorMap
 * @i: an index
 * @col: a color in 32 bit RGBA format
 *
 * Set the ith color in the colormap to a color value.
 **/
void
b_color_map_set (BColorMap *pal, gint i, guint32 col)
{
  g_return_if_fail (B_IS_COLOR_MAP (pal));

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

  g_signal_emit (G_OBJECT (pal), b_color_map_signals[CHANGED], 0);
}

gint
b_color_map_get_offset (BColorMap *pal)
{
  g_return_val_if_fail (B_IS_COLOR_MAP (pal), 0);
  return pal->offset;
}

void
b_color_map_set_offset (BColorMap *pal, gint offset)
{
  gint shift;

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  shift = offset - pal->offset;
  pal->offset = offset;

  if (pal->N > 1) {
    shift = shift % pal->N;
    if (shift < 0)
      shift += pal->N;
    if (shift != 0)
      g_signal_emit (G_OBJECT (pal), b_color_map_signals[CHANGED], 0);
  }
}

gint
b_color_map_get_alpha (BColorMap *pal)
{
  g_return_val_if_fail (B_IS_COLOR_MAP (pal), -1);
  return pal->alpha;
}

void
b_color_map_set_alpha (BColorMap *pal, gint alpha)
{
  g_return_if_fail (B_IS_COLOR_MAP (pal));
  g_return_if_fail (0 <= alpha && alpha <= 255);

  if (pal->alpha != alpha) {
    pal->alpha = alpha;
    g_signal_emit (G_OBJECT (pal), b_color_map_signals[CHANGED], 0);
  }
}

gint
b_color_map_get_intensity (BColorMap *pal)
{
  g_return_val_if_fail (B_IS_COLOR_MAP (pal), -1);
  return pal->intensity;
}

void
b_color_map_set_intensity (BColorMap *pal, gint intensity)
{
  g_return_if_fail (B_IS_COLOR_MAP (pal));
  g_return_if_fail (0 <= intensity && intensity <= 255);
  if (pal->intensity != intensity) {
    pal->intensity = intensity;
    g_signal_emit (G_OBJECT (pal), b_color_map_signals[CHANGED], 0);
  }
}

gboolean
b_color_map_get_flipped (BColorMap *pal)
{
  g_return_val_if_fail (B_IS_COLOR_MAP (pal), FALSE);
  return pal->flip;
}

void
b_color_map_set_flipped (BColorMap *pal, gboolean f)
{
  g_return_if_fail (B_IS_COLOR_MAP (pal));
  if (pal->flip != f) {
    pal->flip = f;
    g_signal_emit (G_OBJECT (pal), b_color_map_signals[CHANGED], 0);
  }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
b_color_map_set_raw (BColorMap *pal, const gchar *meta, guint32 *nodes, gint N, gboolean owned)
{
  g_return_if_fail (B_IS_COLOR_MAP (pal));
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

  g_signal_emit (G_OBJECT (pal), b_color_map_signals[CHANGED], 0);
}

void
b_color_map_set_stock (BColorMap *pal)
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

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  b_color_map_set_raw (pal,
			       palette_info[PALETTE_STOCK].meta,
			       stock_colors,
			       sizeof (stock_colors) / sizeof (guint32),
			       FALSE);
}

void
b_color_map_set_alien_stock (BColorMap *pal)
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

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  b_color_map_set_raw (pal,
			       palette_info[PALETTE_ALIEN_STOCK].meta,
			       alien_stock_colors,
			       sizeof (alien_stock_colors) / sizeof (guint32),
			       FALSE);
}

/**
 * b_color_map_set_transition:
 * @pal: a #BColorMap
 * @c1: a color
 * @c2: another color
 *
 * Set the color map palette to have two colors.
 **/
void
b_color_map_set_transition (BColorMap *pal, guint32 c1, guint32 c2)
{
  guint32 *nodes;

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  nodes = g_new (guint32, 2);
  nodes[0] = c1;
  nodes[1] = c2;

  b_color_map_set_raw (pal, palette_info[PALETTE_TRANSITION].meta, nodes, 2, TRUE);
}

void
b_color_map_set_fade (BColorMap *pal, guint32 c)
{
  guint32 *nodes;

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  nodes = g_new (guint32, 2);
  nodes[0] = 0;
  nodes[1] = c;

  b_color_map_set_raw (pal, palette_info[PALETTE_FADE].meta, nodes, 2, TRUE);
}

void
b_color_map_set_fire (BColorMap *pal)
{
  static guint32 fire_colors[] = {
    0xff000000, 0xff0000ff, 0xffff00ff
  };

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  b_color_map_set_raw (pal,
			       palette_info[PALETTE_FIRE].meta,
			       fire_colors,
			       sizeof (fire_colors) / sizeof (guint32),
			       FALSE);

}

void
b_color_map_set_ice (BColorMap *pal)
{
  static guint32 ice_colors[] = {
    0xff000000, 0xff0000ff, 0xffff00ff
  };

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  b_color_map_set_raw (pal,
			       palette_info[PALETTE_ICE].meta,
			       ice_colors,
			       sizeof (ice_colors) / sizeof (guint32),
			       FALSE);

}

void
b_color_map_set_thermal (BColorMap *pal)
{
  static guint32 thermal_colors[] = {
    0xffffff00, 0xff00ff00, 0xff008080, 0xff0000ff, 0xffff00ff
  };

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  b_color_map_set_raw (pal,
			       palette_info[PALETTE_THERMAL].meta,
			       thermal_colors,
			       sizeof (thermal_colors) / sizeof (guint32),
			       FALSE);
}

void
b_color_map_set_jet (BColorMap *pal)
{
  static guint32 jet_colors[] = {
    0xff000000, 0xff800000, 0xffff0000, 0xffff4000, 0xffff8000, 0xffffc000, 0xffffff00, 0xffc0ff40, 0xff80ff80, 0xff40ffc0, 0xff00ffff, 0xff00c0ff, 0xff0080ff, 0xff0040ff, 0xff0000ff, 0xff000080
  };

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  b_color_map_set_raw (pal,
			       palette_info[PALETTE_JET].meta,
			       jet_colors,
			       sizeof (jet_colors) / sizeof (guint32),
			       FALSE);
}

void
b_color_map_set_seismic (BColorMap *pal)
{
  static guint32 seismic_colors[] = {
    0xff4d0000, 0xffff0000, 0xffffffff, 0xff0000ff, 0xff000080
  };

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  b_color_map_set_raw (pal,
			       palette_info[PALETTE_SEISMIC].meta,
			       seismic_colors,
			       sizeof (seismic_colors) / sizeof (guint32),
			       FALSE);
}

void
b_color_map_set_spectrum (BColorMap *pal)
{
  static guint32 spectrum_colors[] = {
    0xff0000ff, 0xff8000ff, 0xffff00ff, 0x33cc33ff, 0x0080ffff, 0xff80ffff
  };

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  b_color_map_set_raw (pal,
			       palette_info[PALETTE_SPECTRUM].meta,
			       spectrum_colors,
			       sizeof (spectrum_colors) / sizeof (guint32),
			       FALSE);
}

/**
 * b_color_map_set_monochrome:
 * @pal: a #BColorMap
 * @c: a color
 *
 * Set the color map palette to have only one color.
 **/
void
b_color_map_set_monochrome (BColorMap *pal, guint32 c)
{
  guint32 *cc;

  g_return_if_fail (B_IS_COLOR_MAP (pal));

  cc = g_new (guint32, 1);
  *cc = c;
  b_color_map_set_raw (pal,
			       palette_info[PALETTE_MONOCHROME].meta,
			       cc,
			       1,
			       TRUE);
}

/**
 * b_color_map_set_custom:
 * @pal: a #BColorMap
 * @N: the number of colors
 * @color: an array of colors
 *
 * Set the color map palette from an array of colors
 **/
void
b_color_map_set_custom (BColorMap *pal, gint N, guint32 *color)
{
  guint32 *color_cpy;
  gint i;

  g_return_if_fail (B_IS_COLOR_MAP (pal));
  g_return_if_fail (N > 0);

  /* If color is passed in as NULL, set the palette to all black. */

  color_cpy = g_new (guint32, N);
  for (i = 0; i < N; ++i)
    color_cpy[i] = color ? color[i] : RGBA_BLACK;

  b_color_map_set_raw (pal,
			       N > 1 ? palette_info[PALETTE_CUSTOM].meta : palette_info[PALETTE_MONOCHROME].meta,
			       color_cpy, N, TRUE);
}

void
b_color_map_set_vcustom (BColorMap *pal, gint N, guint32 first_color, ...)
{
  guint32 *color;
  gint i;
  va_list args;

  g_return_if_fail (B_IS_COLOR_MAP (pal));
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

  b_color_map_set_raw (pal,
			       N > 1 ? palette_info[PALETTE_CUSTOM].meta : palette_info[PALETTE_MONOCHROME].meta,
			       color, N, TRUE);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
b_color_map_finalize (GObject *obj)
{
  BColorMap *x = B_COLOR_MAP(obj);

  if (x->own_nodes) {
    g_free (x->nodes);
  }
  g_free (x->meta);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

G_DEFINE_TYPE (BColorMap, b_color_map, G_TYPE_OBJECT);

static void
b_color_map_class_init (BColorMapClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = b_color_map_finalize;

  b_color_map_signals[CHANGED] =
    g_signal_new ("changed",
                    G_TYPE_FROM_CLASS(klass),
                    G_SIGNAL_RUN_FIRST,
                    0,NULL,NULL,
                    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
b_color_map_init (BColorMap *pal)
{
  pal->meta      = g_strdup ("custom");
  pal->alpha     = 0xff;
  pal->intensity = 0xff;
}
