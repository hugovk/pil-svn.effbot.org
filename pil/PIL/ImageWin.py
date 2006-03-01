#
# The Python Imaging Library.
# $Id$
#
# a Windows DIB display interface
#
# History:
#       96-05-20 fl     Created
#       96-09-20 fl     Fixed subregion exposure
#       97-09-21 fl     Added draw primitive (for tzPrint)
#
# Copyright (c) Secret Labs AB 1997.
# Copyright (c) Fredrik Lundh 1996-97.
#
# See the README file for information on usage and redistribution.
#

import Image

#
# Class wrapper for the Windows display buffer interface
#
# Create an object of this type, paste your data into it, and call
# expose with an hDC casted to a Python int...  In PythonWin, you
# can use the GetHandleAttrib() method of the CDC class to get an
# appropriate hDC.
#

class Dib:

    def __init__(self, mode, size):
        if mode not in ["1", "L", "P", "RGB"]:
            mode = "RGB"
        self.image = Image.core.display(mode, size)
        self.mode = mode
        self.size = size

    def expose(self, dc):
        return self.image.expose(dc)

    def draw(self, dc, dst, src = None):
        if not src:
            src = (0,0) + self.size
        return self.image.draw(dc, dst, src)

    def query_palette(self, dc):
        return self.image.query_palette(dc)

    def paste(self, im, box = None):
        # fix to handle conversion when pasting
        im.load()
        if self.mode != im.mode:
            im = im.convert(self.mode)
        if box:
            self.image.paste(im.im, box)
        else:
            self.image.paste(im.im)
