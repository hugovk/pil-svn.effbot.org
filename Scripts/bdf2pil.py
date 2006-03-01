#! /usr/local/bin/python
#
# $Id: //modules/pil/Scripts/bdf2pil.py#2 $
#
# File:
#       bdf2pil.py -- font compiler for X bitmap distribution files
#
# Description:
#       This utility converts a BDF font to an image file with an
#       associated font metrics file.
#
# History:
#       96-05-16 fl:    Created (after an idea from David Ascher)
#       96-08-07 fl:    Font metrics file uses network byte order
#
# Copyright (c) Fredrik Lundh 1996.  All rights reserved.
#

VERSION = "1.0"


# HACK
try:
    import Image
except:
    import sys
    sys.path.insert(0, ".")
    import Image


# --------------------------------------------------------------------
# Parse X Bitmap Distribution Format (BDF)
# --------------------------------------------------------------------

import string

bdf_slant = {
   "R": "Roman",
   "I": "Italic",
   "O": "Oblique",
   "RI": "Reverse Italic",
   "RO": "Reverse Oblique",
   "OT": "Other"
}

bdf_spacing = {
    "P": "Proportional",
    "M": "Monospaced",
    "C": "Cell"
}

def bdf_char(f):

    # skip to STARTCHAR
    while 1:
        s = f.readline()
        if not s:
            return None
        if s[:9] == "STARTCHAR":
            break
    id = string.strip(s[9:])

    # load symbol properties
    props = {}
    while 1:
        s = f.readline()
        if not s or s[:6] == "BITMAP":
            break
        i = string.find(s, " ")
        props[s[:i]] = s[i+1:-1]

    # load bitmap
    bitmap = []
    while 1:
        s = f.readline()
        if not s or s[:7] == "ENDCHAR":
            break
        bitmap.append(s[:-1])
    bitmap = string.join(bitmap, "")

    [w, h, x, y] = map(string.atoi, string.split(props["BBX"]))
    [dx, dy] = map(string.atoi, string.split(props["DWIDTH"]))

    bbox = (dx, dy, x, y, x+w, y+h)

    return id, string.atoi(props["ENCODING"]), bbox, bitmap


def bdf2font(filename):

    f = open(filename)

    s = f.readline()
    if s[:13] != "STARTFONT 2.1":
        print filename, "is not a valid BDF file (ignoring)"
        return None

    print filename

    props = {}
    comments = []

    while 1:
        s = f.readline()
        if not s or s[:13] == "ENDPROPERTIES":
            break
        i = string.find(s, " ")
        props[s[:i]] = s[i+1:-1]
        if s[:i] in ["COMMENT", "COPYRIGHT"]:
            if string.find(s, "LogicalFontDescription") < 0:
                comments.append(s[i+1:-1])

    font = string.split(props["FONT"], "-")

    font[4] = bdf_slant[font[4]]
    font[11] = bdf_spacing[font[11]]

    ascent = string.atoi(props["FONT_ASCENT"])
    descent = string.atoi(props["FONT_DESCENT"])

    fontname = string.join(font[1:], ";")

    print "#", fontname
    #for i in comments:
    #   print "#", i

    font = []
    while 1:
        c = bdf_char(f)
        if not c:
            break
        id, code, bbox, bits = c
        if code >= 0:
            font.append((code, bbox, bits))

    return font, fontname, ascent, descent


# --------------------------------------------------------------------
# Translate font to image
# --------------------------------------------------------------------

def font2image(font, ascent, descent):

    metrics = [None] * 256

    # create glyph images for all characters in this font
    width = height = 0
    glyph = [None]*256
    for id, (dx, dy, b0, b1, b2, b3), bits in font:
        w, h = b2-b0, b3-b1
        if w > 0 and h > 0:
            i = Image.core.new("L", (w, h))
            height = max(height, h)
            d = Image.core.hex_decoder("1")
            d.setimage(i)
            d.decode(bits)
            glyph[id] = i
            width = width + w

    # pack the glyphs into a large image
    x = 0
    bboxes = {}
    i = Image.core.fill("L", (width, height), 0)
    for id, (dx, dy, b0, b1, b2, b3), bits in font:
        w, h = b2-b0, b3-b1
        if w > 0 and h > 0:
            bbox = (x, 0, x+w, h)
            metrics[id] = (dx, dy), (b0, b1, b2, b3), bbox
            i.paste(glyph[id], bbox)
            x = x + w
            bboxes[id] = bbox

    return i, bboxes

def puti16(fp, values):
    # write network order (big-endian) 16-bit sequence
    for v in values:
        fp.write(chr(v>>8&255) + chr(v&255))

# --------------------------------------------------------------------
# MAIN
# --------------------------------------------------------------------

import glob, os, sys

print "BDF2PIL", VERSION, "-- Font compiler for X BDF fonts."
print "Copyright (c) Fredrik Lundh 1996.  All rights reserved."

if len(sys.argv) <= 1:
    print
    print "Usage: bdf2pil files..."
    print
    print "BDF files are converted to a bitmap file (currently"
    print "using PBM format) and a font metrics file (PIL)."
    sys.exit(1)

files = []
for f in sys.argv[1:]:
    files = files + glob.glob(f)

for file in files:
    font, fontname, ascent, descent = bdf2font(file)
    if font:
        image, srcbbox = font2image(font, ascent, descent)
        # to be changed to PCX or something...
        image.save_ppm(os.path.splitext(file)[0] + ".pbm")
        # create font metrics file
        metrics = {}
        for id, xy_bbox, bits in font:
            x, y, x0, y0, x1, y1 = xy_bbox
            metrics[id] = (x, y, x0, ascent-y1, x1, ascent-y0) + srcbbox[id]
        fp = open(os.path.splitext(file)[0] + ".pil", "wb")
        fp.write("PILfont\n")
        fp.write("%s\n" % fontname)
        fp.write("DATA\n")
        for id in range(256):
            try:
                puti16(fp, metrics[id])
            except KeyError:
                puti16(fp, (0,) * 10)
        fp.close()

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

