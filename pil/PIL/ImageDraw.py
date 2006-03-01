#
# The Python Imaging Library
# $Id$
#
# drawing interface operations
#
# History:
# 1996-04-13 fl   Created (experimental)
# 1996-08-07 fl   Filled polygons, ellipses.
# 1996-08-13 fl   Added text support
# 1998-06-28 fl   Handle I and F images
# 1998-12-29 fl   Added arc; use arc primitive to draw ellipses
# 1999-01-10 fl   Added shape stuff (experimental)
# 1999-02-06 fl   Added bitmap support
# 1999-02-11 fl   Changed all primitives to take options
# 1999-02-20 fl   Fixed backwards compatibility
# 2000-10-12 fl   Copy on write, when necessary
# 2001-02-18 fl   Use default ink for bitmap/text also in fill mode
#
# Copyright (c) 1997-2001 by Secret Labs AB
# Copyright (c) 1996-2001 by Fredrik Lundh
#
# See the README file for information on usage and redistribution.
#

import Image

class Draw:

    def __init__(self, im):
        im.load()
        if im.readonly:
            im._copy() # make it writable
        self.im = im.im
        if im.mode in ("I", "F"):
            self.ink = self.im.draw_ink(1)
        else:
            self.ink = self.im.draw_ink(-1)
        self.fill = 0
        self.font = None

    #
    # attributes

    def setink(self, ink):
        # compatibility
        self.ink = self.im.draw_ink(ink)

    def setfill(self, onoff):
        # compatibility
        self.fill = onoff

    def setfont(self, font):
        # compatibility
        self.font = font

    def getfont(self):
        if not self.font:
            # FIXME: should add a font repository
            import ImageFont
            self.font = ImageFont.load_path("BDF/courR14.pil")
        return self.font

    def textsize(self, text, font=None):
        if font is None:
            font = self.getfont()
        return font.getsize(text)

    def _getink(self, ink, fill=None):
        if ink is None and fill is None:
            if self.fill:
                fill = self.ink
            else:
                ink = self.ink
        else:
            if ink is not None:
                ink = self.im.draw_ink(ink)
            if fill is not None:
                fill = self.im.draw_ink(fill)
        return ink, fill

    #
    # primitives

    def arc(self, xy, start, end, fill=None):
        ink, fill = self._getink(fill)
        if ink is not None:
            self.im.draw_arc(xy, start, end, ink)

    def bitmap(self, xy, bitmap, fill=None):
        bitmap.load()
        ink, fill = self._getink(fill)
        if ink is None:
            ink = fill
        if ink is not None:
            self.im.draw_bitmap(xy, bitmap.im, ink)

    def chord(self, xy, start, end, fill=None, outline=None):
        ink, fill = self._getink(outline, fill)
        if fill is not None:
            self.im.draw_chord(xy, start, end, fill, 1)
        if ink is not None:
            self.im.draw_chord(xy, start, end, ink, 0)

    def ellipse(self, xy, fill=None, outline=None):
        ink, fill = self._getink(outline, fill)
        if fill is not None:
            self.im.draw_ellipse(xy, fill, 1)
        if ink is not None:
            self.im.draw_ellipse(xy, ink, 0)

    def line(self, xy, fill=None):
        ink, fill = self._getink(fill)
        if ink is not None:
            self.im.draw_lines(xy, ink)

    def shape(self, shape, fill=None, outline=None):
        # experimental
        shape.close()
        ink, fill = self._getink(outline, fill)
        if fill is not None:
            self.im.draw_outline(shape, fill, 1)
        if ink is not None:
            self.im.draw_outline(shape, ink, 0)

    def pieslice(self, xy, start, end, fill=None, outline=None):
        ink, fill = self._getink(outline, fill)
        if fill is not None:
            self.im.draw_pieslice(xy, start, end, fill, 1)
        if ink is not None:
            self.im.draw_pieslice(xy, start, end, ink, 0)

    def point(self, xy, fill=None):
        ink, fill = self._getink(fill)
        if ink is not None:
            self.im.draw_points(xy, ink)

    def polygon(self, xy, fill=None, outline=None):
        ink, fill = self._getink(outline, fill)
        if fill is not None:
            self.im.draw_polygon(xy, fill, 1)
        if ink is not None:
            self.im.draw_polygon(xy, ink, 0)

    def rectangle(self, xy, fill=None, outline=None):
        ink, fill = self._getink(outline, fill)
        if fill is not None:
            self.im.draw_rectangle(xy, fill, 1)
        if ink is not None:
            self.im.draw_rectangle(xy, ink, 0)

    def text(self, xy, text, fill=None, font=None, anchor=None):
        ink, fill = self._getink(fill)
        if font is None:
            font = self.getfont()
        if ink is None:
            ink = fill
        if ink is not None:
            self.im.draw_bitmap(xy, font.getmask(text), ink)

# backwards compatibility
ImageDraw = Draw

# experimental access to the outline API
try:
    Outline = Image.core.outline
except:
    Outline = None
