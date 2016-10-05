/*
 * y-density-plot.h
 *
 * Copyright (C) 2016 Scott O. Johnson (scojo202@gmail.com)
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
 
#include <gtk/gtk.h>
#include <y-data.h>
#include "y-element-view-cartesian.h"

#ifndef __DENSITY_PLOT_H__
#define __DENSITY_PLOT_H__

G_BEGIN_DECLS

typedef struct _YDensityPlot YDensityPlot;
typedef struct _YDensityPlotClass YDensityPlotClass;

struct _YDensityPlot {
  YElementViewCartesian parent;
  
  //cairo_surface_t *surf;
  
  GdkPixbuf *pixbuf, *scaled_pixbuf;
  
  YMatrix * tdata;
  gulong tdata_changed_id;
  
  double xmin,dx;
  double ymin,dy;
  
  double zmax;
  gboolean auto_z;
  
  double scalex, scaley;
  float aspect_ratio;
  gboolean preserve_aspect;
  
  gboolean draw_line;
  GtkOrientation line_dir;
  int line_pos, line_width;
  
  gboolean draw_dot;
  double dot_pos_x, dot_pos_y;
};

struct _YDensityPlotClass {
  YElementViewCartesianClass parent_class;
};

#define Y_TYPE_DENSITY_PLOT            (y_density_plot_get_type ())
#define Y_DENSITY_PLOT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), Y_TYPE_DENSITY_PLOT, YDensityPlot))
#define Y_DENSITY_PLOT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), Y_TYPE_DENSITY_PLOT, YDensityPlotClass))
#define Y_IS_DENSITY_PLOT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), Y_TYPE_DENSITY_PLOT))
#define Y_IS_DENSITY_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), Y_TYPE_DENSITY_PLOT))
#define Y_DENSITY_PLOT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), Y_TYPE_DENSITY_PLOT, YDensityPlotClass))

GType y_density_plot_get_type (void);

G_END_DECLS

#endif

