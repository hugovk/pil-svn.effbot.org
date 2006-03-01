#
# The Python Imaging Library.
# $Id: //modules/pil/PIL/GdImageFile.py#3 $
#
# GD file handling
#
# History:
#       96-04-12 fl     Created
#
# Copyright (c) Secret Labs AB 1997.
# Copyright (c) Fredrik Lundh 1996.
#
# See the README file for information on usage and redistribution.
#


# NOTE: This format cannot be automatically recognized, so the
# class is not registered for use with Image.open().  To open a
# gd file, use the GdImageFile.open() function instead.

# THE GD FORMAT IS NOT DESIGNED FOR DATA INTERCHANGE.  This
# implementation is provided for convenience and demonstrational
# purposes only.


__version__ = "0.1"

import string
import Image, ImageFile, ImagePalette

def i16(c):
    return ord(c[1]) + (ord(c[0])<<8)


class GdImageFile(ImageFile.ImageFile):

    format = "GD"
    format_description = "GD uncompressed images"

    def _open(self):

        # Header
        s = self.fp.read(775)

        self.mode = "L" # FIXME: "P"
        self.size = i16(s[0:2]), i16(s[2:4])

        # transparency index
        tindex = i16(s[5:7])
        if tindex < 256:
            self.info["transparent"] = tindex

        self.palette = ImagePalette.raw("RGB", s[7:])

        self.tile = [("raw", (0,0)+self.size, 775, ("L", 0, -1))]


def open(fp, mode = "r"):
    "Open a GD image file, without loading the raster data"

    if mode != "r":
        raise ValueError, "bad mode"

    if type(fp) == type(""):
        import __builtin__
        filename = fp
        fp = __builtin__.open(fp, "rb")
    else:
        filename = ""

    try:
        return GdImageFile(fp, filename)
    except SyntaxError:
        raise IOError, "cannot identify this image file"

# save is not supported
