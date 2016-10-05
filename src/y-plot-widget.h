/*
 * y-plot-widget.h
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
#include "y-axis-view.h"
#include "y-scatter-view.h"

#ifndef _INC_PLOT_WIDGET_H
#define _INC_PLOT_WIDGET_H

G_BEGIN_DECLS

#define TYPE_PLOT_WIDGET (plot_widget_get_type())
#define PLOT_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),TYPE_PLOT_WIDGET,PlotWidget))
#define PLOT_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),TYPE_PLOT_WIDGET,PlotWidgetClass))
#define IS_PLOT_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_PLOT_WIDGET))
#define IS_PLOT_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_PLOT_WIDGET))

GType plot_widget_get_type (void);

typedef struct {
    YVector *xdata;
    YVector * ydata;
    YScatterView *view;
} SeqPair;

typedef struct _PlotWidget PlotWidget;
typedef struct _PlotWidgetClass PlotWidgetClass;
struct _PlotWidgetPrivate;

struct _PlotWidget {
  GtkEventBox parent;
  struct _PlotWidgetPrivate *priv;

  YAxisView * north_axis;
  YAxisView * south_axis;
  YAxisView * west_axis;
  YAxisView * east_axis;
  
  YElementViewCartesian *main_view;
  
  GSList * series;
};

struct _PlotWidgetClass {
  GtkEventBoxClass parent_class;

};

void plot_widget_add_view(PlotWidget *plot, YElementViewCartesian *view);

YScatterView * plot_widget_add_line_data (PlotWidget * plot, YVector  * x, YVector  * y);

void plot_widget_freeze(PlotWidget *plot);
void plot_widget_thaw(PlotWidget *plot);

void plot_widget_set_max_frame_rate(PlotWidget *plot, float rate);

gboolean plot_widget_draw_pending(PlotWidget *plot);

G_END_DECLS

#endif
