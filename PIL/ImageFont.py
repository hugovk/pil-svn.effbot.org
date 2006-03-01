#
# The Python Imaging Library.
# $Id$
#
# PIL raster font management
#
# History:
# 1996-08-07 fl   created (experimental)
# 1997-08-25 fl   minor adjustments to handle fonts from pilfont 0.3
# 1999-02-06 fl   rewrote most font management stuff in C
# 1999-03-17 fl   take pth files into account in load_path (from Richard Jones)
# 2001-02-17 fl   added freetype support
#
# Todo:
# Adapt to PILFONT2 format (16-bit fonts, compressed, single file)
#
# Copyright (c) Secret Labs AB 1997-2001
# Copyright (c) Fredrik Lundh 1996-2001
#
# See the README file for information on usage and redistribution.
#

import Image
import os, string, sys

# --------------------------------------------------------------------
# Font metrics format:
#       "PILfont" LF
#       fontdescriptor LF
#       (optional) key=value... LF
#       "DATA" LF
#       binary data: 256*10*2 bytes (dx, dy, dstbox, srcbox)
#
# To place a character, cut out srcbox and paste at dstbox,
# relative to the character position.  Then move the character
# position according to dx, dy.
# --------------------------------------------------------------------

# FIXME: add support for pilfont2 format (see FontFile.py)

class ImageFont:
    "PIL font wrapper"

    def _load_pilfont(self, filename):

        fp = open(filename, "rb")

        # read PILfont header
        if fp.readline() != "PILfont\n":
            raise SyntaxError, "Not a PILfont file"
        d = string.split(fp.readline(), ";")
        self.info = [] # FIXME: should be a dictionary
        s = fp.readline()
        while s and s != "DATA\n":
            self.info.append(s)

        # read PILfont metrics
        data = fp.read(256*20)

        # read PILfont bitmap
        image = None
        for ext in (".png", ".gif", ".pbm"):
            try:
                image = Image.open(os.path.splitext(filename)[0] + ext)
            except:
                pass
            if image:
                break
        if image is None:
            raise IOError, "cannot find glyph data file"

        image.load()
        self.font = Image.core.font(image.im, data)

        # delegate critical operations to internal type
        self.getsize = self.font.getsize
        self.getmask = self.font.getmask


class FreeTypeFont:
    "FreeType font wrapper (requires _imagingft service)"

    def __init__(self, file, size, index=0):
        # FIXME: use service provider instead
        import _imagingft
        self.font = _imagingft.getfont(file, size, index)

    def getsize(self, text):
        return self.font.getsize(text)

    def getmask(self, text, fill=Image.core.fill):
        size = self.font.getsize(text)
        im = fill("L", size, 0)
        self.font.render(text, im.id)
        return im

#
# --------------------------------------------------------------------

def load(filename):
    "Load a font file."
    f = ImageFont()
    f._load_pilfont(filename)
    return f

def truetype(filename, size, index=0):
    "Load a truetype font file."
    try:
        return FreeTypeFont(filename, size, index)
    except IOError:
        if sys.platform == "win32":
            # check the windows font repository
            # NOTE: must use uppercase WINDIR, to work around bugs in
            # 1.5.2's os.environ.get()
            windir = os.environ.get("WINDIR")
            if windir:
                filename = os.path.join(windir, "fonts", filename)
                return FreeTypeFont(filename, size, index)
        raise

def load_path(filename):
    "Load a font file, searching along the Python path."
    for dir in sys.path:
        try:
            return load(os.path.join(dir, filename))
        except IOError:
            pass
        # disabled: site.py already does this for us
##         import glob
##         for extra in glob.glob(os.path.join(dir, "*.pth")):
##             try:
##                 dir = os.path.join(dir, string.strip(open(extra).read()))
##                 return load(os.path.join(dir, filename))
##             except IOError:
##                 pass
    raise IOError, "cannot find font file"
