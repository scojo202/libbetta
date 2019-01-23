# LibBetta

LibBetta is a GTK3-based C library for plotting, with an emphasis on interactivity
rather than production of publication-quality graphics.

It is based on code from GUPPI and GOffice.

LibBetta is approaching usefulness but is under active development. The API is not
stable.

## Interactivity

Current functionality:

* When the cursor is over a scatter/line plot, the position is shown in the
toolbar
* In zoom mode, the scroll wheel zooms in and out around the cursor position.
* In zoom mode, clicks zoom in at the cursor position and ALT-click zooms out.
* In zoom mode, holding the mouse button and dragging creates a zoom box.
Releasing the button zooms into the box.
* In pan mode, holding the mouse button and dragging translates the plot.
* In pan mode, shift-click centers the plot on the cursor position.
* Context menu controls whether views are autoscaled or not.
