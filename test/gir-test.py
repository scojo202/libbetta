import gi
gi.require_version("YPlot","0.2")
gi.require_version("Gtk","3.0")
from gi.repository import YPlot, Gtk

a=YPlot.ValVector.new_copy([3,4,5])
b=YPlot.ValVector.new_copy([1,2,3])

w=Gtk.Window()
w.set_default_size(300,400);
w.connect("delete-event",Gtk.main_quit)

series=YPlot.ScatterSeries()
series.set_data("x",b)
series.set_data("y",a)

scatter_plot = YPlot.ScatterLinePlot()

scatter_view = scatter_plot.main_view

xaxis_view = scatter_plot.south_axis
xaxis_view.set_property("axis_label","x axis")

yaxis_view = scatter_plot.west_axis
yaxis_view.set_property("axis_label","y axis")

scatter_view.add_series(series)

w.add(scatter_plot)

w.show_all()

Gtk.main()