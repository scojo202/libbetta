import gi
gi.require_version("Betta","0.2")
gi.require_version("Gtk","3.0")
from gi.repository import Betta, Gtk

import numpy as np

DATA_COUNT = 200

t=2*np.pi*np.arange(DATA_COUNT)/DATA_COUNT

d1 = Betta.ValVector.new_copy(2*np.sin(4*t))
d2 = Betta.ValVector.new_copy(np.cos(3*t))
d3 = Betta.ValVector.new_copy(np.cos(5*t))

w=Gtk.Window()
w.set_default_size(300,400)
w.connect("delete-event",Gtk.main_quit)

series1=Betta.ScatterSeries()
series1.set_property("x-data",d1)
series1.set_property("y-data",d2)
series1.set_line_color_from_string("#ff0000")

series2=Betta.ScatterSeries()
series2.set_property("x-data",d1)
series2.set_property("y-data",d3)

scatter_plot = Betta.PlotWidget.new_scatter(series1)

scatter_view = scatter_plot.get_main_view()

scatter_plot.set_x_label("x axis")
scatter_plot.set_y_label("y axis")

scatter_view.add_series(series2)

box = Gtk.VBox()
box.pack_start(Gtk.Label("title"),True,True,0)
box.pack_start(scatter_plot,True,True,0)

w.add(box)

w.show_all()

Gtk.main()
