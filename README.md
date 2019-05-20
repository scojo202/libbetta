# LibBetta

LibBetta is a GTK3-based C library for plotting, with an emphasis on interactivity and speed
rather than production of publication-quality graphics.

It is based on code from GUPPI and GOffice and has no dependencies beyond GTK3.

LibBetta is approaching usefulness but is under active development. The API is not
yet stable. API reference documentation can be found [here](https://scojo202.github.io/libbetta/docs).

## Interactivity

Current functionality:

* When the cursor is over the plot area, the position is shown in the toolbar.
* In zoom mode, the scroll wheel zooms in and out around the cursor position.
* In zoom mode, clicks zoom in at the cursor position and ALT-click zooms out.
* In zoom mode, holding the mouse button and dragging creates a zoom box.
Releasing the button zooms into the box.
* In pan mode, holding the mouse button and dragging translates the plot.
* In pan mode, shift-click centers the plot on the cursor.
* In scatter/line plot, vertical and horizontal cursors, controllable interactively.
* Context menu controls whether or not axes are autoscaled and whether cursors are visible.
* Save to PNG, PDF, and SVG.

## Installation

Compilation is done using [meson](http://mesonbuild.com). To clone and compile, do something like the following:

    $ git clone https://github.com/scojo202/libbetta
    $ cd libbetta
    $ meson build  # may want to use --prefix to set installation directory
    $ cd build
    $ ninja
    # ninja install

LibBetta has been successfully compiled and used in Linux and MacOS X.
