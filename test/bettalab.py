import gi
gi.require_version("Betta","0.2")
gi.require_version("Gtk","3.0")
from gi.repository import Betta, Gtk

import numpy as np

figuredict={} # list of currently open figures
curr_fig=-1

def on_fig_closed(widget,event,i):
  global curr_fig
  del figuredict[i]
  curr_fig=-1

def gcf():
  return curr_fig

def _make_new_figure(i):
  w=Gtk.Window()
  w.set_title("Figure "+str(i))
  w.connect("delete-event",on_fig_closed,i)
  w.set_default_size(400,500)
  return w

def figure(i):
  global curr_fig
  if curr_fig==i:
    w=figuredict[curr_fig]
    w.present()
  elif i in figuredict.keys():
    w=figuredict[i]
    w.present()
  else:
    w=_make_new_figure(i)
    figuredict[i]=w
    w.show_all()
  curr_fig = i

def plot(x,y):
  global curr_fig
  series1=Betta.ScatterSeries()
  series1.set_x_array(x)
  series1.set_y_array(y)
  series1.set_line_color_from_string("#0000ff")
  if curr_fig<0: # make a new figure first
    figure(1)
  if figuredict[curr_fig].get_child() is None: # empty figure
    scatter_plot = Betta.PlotWidget.new_scatter(series1)
    figuredict[curr_fig].add(scatter_plot)
  else:
    scatter_plot = figuredict[curr_fig].get_child()
    scatter_view = scatter_plot.get_main_view()
    scatter_view.add_series(series1)
  figuredict[curr_fig].show_all()
