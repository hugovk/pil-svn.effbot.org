#
# The Python Imaging Library.
# $Id: //modules/pil/PIL/PcxImagePlugin.py#3 $
#
# PCX file handling
#
# This format was originally used by ZSoft's popular PaintBrush
# program for the IBM PC.  It is also supported by many MS-DOS and
# Windows applications, including the Windows PaintBrush program in
# Windows 3.
#
# history:
# 95-09-01 fl   Created
# 96-05-20 fl   Fixed RGB support
# 97-01-03 fl   Fixed 2-bit and 4-bit support
# 99-02-03 fl   Fixed 8-bit support (broken in 1.0b1)
# 99-02-07 fl   Added write support
#
# Copyright (c) Secret Labs AB 1997-98.
# Copyright (c) Fredrik Lundh 1995-97.
#
# See the README file for information on usage and redistribution.
#

__version__ = "0.4"

import Image, ImageFile, ImagePalette

def i16(c):
    return ord(c[0]) + (ord(c[1])<<8)

def _accept(prefix):
    return ord(prefix[0]) == 10 and ord(prefix[1]) in [0, 2, 3, 5]

class PcxImageFile(ImageFile.ImageFile):

    format = "PCX"
    format_description = "Paintbrush"

    def _open(self):

        # header
        s = self.fp.read(128)
        if not _accept(s):
            raise SyntaxError, "not a PCX file"


        # image
        bbox = i16(s[4:]), i16(s[6:]), i16(s[8:])+1, i16(s[10:])+1
        if bbox[2] <= bbox[0] or bbox[3] <= bbox[1]:
            raise SyntaxError, "bad PCX image size"

        # format
        version = ord(s[1])
        bits = ord(s[3])
        planes = ord(s[65])
        stride = i16(s[66:])

        if bits == 1 and planes == 1:
            mode = rawmode = "1"

        elif bits == 1 and planes in (2, 4):
            mode = "P"
            rawmode = "P;%dL" % planes
            self.palette = ImagePalette.raw("RGB", s[16:64])

        elif version == 5 and bits == 8 and planes == 1:
            mode = rawmode = "L"
            # FIXME: hey, this doesn't work with the incremental loader !!!
            self.fp.seek(-769, 2)
            s = self.fp.read(769)
            if len(s) == 769 and ord(s[0]) == 12:
                # check if the palette is linear greyscale
                for i in range(256):
                    if s[i*3+1:i*3+4] != chr(i)*3:
                        mode = rawmode = "P"
                        break
                if mode == "P":
                    self.palette = ImagePalette.raw("RGB", s[1:])

        elif version == 5 and bits == 8 and planes == 3:
            mode = "RGB"
            rawmode = "RGB;L"

        else:
            raise IOError, "unknown PCX mode"

        self.mode = mode
        self.size = bbox[2]-bbox[0], bbox[3]-bbox[1]

        self.tile = [("pcx", bbox, 128, rawmode)]


# --------------------------------------------------------------------
# save PCX files

SAVE = {
    # mode: (version, bits, planes, raw mode)
    "1": (2, 1, 1, "1"),
    "L": (5, 8, 1, "L"),
    "P": (5, 8, 1, "P"),
    "RGB": (5, 8, 3, "RGB;L"),
}

def o16(i):
    return chr(i&255) + chr(i>>8&255)

def _save(im, fp, filename, check=0):

    try:
        version, bits, planes, rawmode = SAVE[im.mode]
    except KeyError:
        raise ValueError, "Cannot save %s images as PCX" % im.mode

    if check:
        return check

    # bytes per plane
    stride = (im.size[0] * bits + 7) / 8

    # under windows, we could determine the current screen size with
    # "Image.core.display_mode()[1]", but I think that's overkill...

    screen = im.size

    dpi = 100, 100

    # PCX header
    fp.write(
        chr(10) + chr(version) + chr(1) + chr(bits) + o16(0) +
        o16(0) + o16(im.size[0]-1) + o16(im.size[1]-1) + o16(dpi[0]) +
        o16(dpi[1]) + chr(0)*24 + chr(255)*24 + chr(0) + chr(planes) +
        o16(stride) + o16(1) + o16(screen[0]) + o16(screen[1]) +
        chr(0)*54
        )

    assert fp.tell() == 128

    ImageFile._save(im, fp, [("pcx", (0,0)+im.size, 0,
                              (rawmode, bits*planes))])

    if im.mode == "P":
        # colour palette
        fp.write(chr(12))
        fp.write(im.im.getpalette("RGB", "RGB")) # 768 bytes
    elif im.mode == "L":
        # greyscale palette
        fp.write(chr(12))
        for i in range(256):
            fp.write(chr(i)*3)

# --------------------------------------------------------------------
# registry

Image.register_open("PCX", PcxImageFile, _accept)
Image.register_save("PCX", _save)

Image.register_extension("PCX", ".pcx")
