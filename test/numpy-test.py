import gi
gi.require_version("YPlot","0.2")
gi.require_version("Gtk","3.0")
from gi.repository import YPlot, Gtk

import numpy as np

DATA_COUNT = 200

t=2*np.pi*np.arange(DATA_COUNT)/DATA_COUNT

d1 = YPlot.ValVector.new_copy(2*np.sin(4*t))
d2 = YPlot.ValVector.new_copy(np.cos(3*t))
d3 = YPlot.ValVector.new_copy(np.cos(5*t))

w=Gtk.Window()
w.set_default_size(300,400);
w.connect("delete-event",Gtk.main_quit)

series1=YPlot.ScatterSeries()
series1.set_property("x-data",d1)
series1.set_property("y-data",d2)
series1.set_line_color_from_string("#ff0000")

series2=YPlot.ScatterSeries()
series2.set_property("x-data",d1)
series2.set_property("y-data",d3)

scatter_plot = YPlot.ScatterLinePlot()

scatter_view = scatter_plot.main_view

scatter_plot.south_axis.set_property("axis_label","x axis")
scatter_plot.west_axis.set_property("axis_label","y axis")

scatter_view.add_series(series1)
scatter_view.add_series(series2)

w.add(scatter_plot)

w.show_all()

Gtk.main()
