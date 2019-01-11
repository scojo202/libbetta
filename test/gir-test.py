import gi
gi.require_version("YPlot","0.2")
gi.require_version("Gtk","3.0")
from gi.repository import YPlot, Gtk

import math

DATA_COUNT = 200

x=[]
y=[]
z=[]
i=0.0
while i<DATA_COUNT:
  t = 2*math.pi*i/DATA_COUNT;
  x.append(2*math.sin(4*t))
  y.append(math.cos(3*t))
  z.append(math.cos(5*t))
  i=i+1

w=Gtk.Window()
w.set_default_size(300,400);
w.connect("delete-event",Gtk.main_quit)

series1=YPlot.ScatterSeries()
series1.set_x_array(x)
series1.set_y_array(y)
series1.set_line_color_from_string("#ff0000")

series2=YPlot.ScatterSeries()
series2.set_x_array(x)
series2.set_y_array(z)

scatter_plot = YPlot.PlotWidget.new_scatter(series1)

scatter_plot.set_x_label("x axis")
scatter_plot.set_y_label("y axis")

main_view = scatter_plot.get_main_view()
main_view.add_series(series2)

w.add(scatter_plot)

w.show_all()

Gtk.main()
