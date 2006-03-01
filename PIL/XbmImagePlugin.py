#
# The Python Imaging Library.
# $Id$
#
# XBM File handling
#
# History:
#	95-09-08 fl	Created
#	96-11-01 fl	Added save support
#	97-07-07 fl	Made header parser more tolerant
#	97-07-22 fl	Fixed yet another parser bug
#
# Copyright (c) Secret Labs AB 1997.
# Copyright (c) Fredrik Lundh 1996-97.
#
# See the README file for information on usage and redistribution.
#

__version__ = "0.3"

import regex, string
import Image, ImageFile

# XBM header
xbm_head = regex.compile(
    "#define[ \t]+[^_]*_width[ \t]+\([0-9]*\)[\r\n]+"
    "#define[ \t]+[^_]*_height[ \t]+\([0-9]*\)[\r\n]+"
    "[\000-\377]*_bits\[\]"
)


def _accept(prefix):
    return prefix[:7] == "#define"


class XbmImageFile(ImageFile.ImageFile):

    format = "XBM"
    format_description = "X11 Bitmap"

    def _open(self):

	s = xbm_head.match(self.fp.read(512))

	if s > 0:

	    xsize = string.atoi(xbm_head.group(1))
	    ysize = string.atoi(xbm_head.group(2))

	    self.mode = "1"
	    self.size = xsize, ysize

	    self.tile = [("xbm", (0, 0)+self.size, s, None)]


def _save(im, fp, filename):

    if im.mode != "1":
	raise IOError, "cannot write mode %s as XBM" % im.mode

    fp.write("#define im_width %d\n" % im.size[0])
    fp.write("#define im_height %d\n" % im.size[1])
    fp.write("static char im_bits[] = {\n")

    ImageFile._save(im, fp, [("xbm", (0,0)+im.size, 0, None)])

    fp.write("};\n")


Image.register_open("XBM", XbmImageFile, _accept)
Image.register_save("XBM", _save)

Image.register_extension("XBM", ".xbm")

Image.register_mime("XBM", "image/xbm")
