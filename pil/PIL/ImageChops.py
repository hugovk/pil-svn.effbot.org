#
# The Python Imaging Library.
# $Id: //modules/pil/PIL/ImageChops.py#2 $
#
# standard channel operations
#
# History:
# 1996-03-24 fl   Created
# 1996-08-13 fl   Added logical operations (for "1" images)
# 2000-10-12 fl   Added offset method (from Image.py)
#
# Copyright (c) 1997-2000 by Secret Labs AB
# Copyright (c) 1996-2000 by Fredrik Lundh
#
# See the README file for information on usage and redistribution.
#

import Image

def constant(image, value):
    "Fill a channel with a given grey level"

    return Image.new("L", image.size, value)

def duplicate(image):
    "Create a copy of a channel"

    return image.copy()

def invert(image):
    "Invert a channel"

    image.load()
    return image._new(image.im.chop_invert())

def lighter(image1, image2):
    "Select the lighter pixels from each image"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_lighter(image2.im))

def darker(image1, image2):
    "Select the darker pixels from each image"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_darker(image2.im))

def difference(image1, image2):
    "Subtract one image from another"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_difference(image2.im))

def multiply(image1, image2):
    "Superimpose two positive images"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_multiply(image2.im))

def screen(image1, image2):
    "Superimpose two negative images"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_screen(image2.im))

def add(image1, image2, scale=1.0, offset=0):
    "Add two images"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_add(image2.im, scale, offset))

def subtract(image1, image2, scale=1.0, offset=0):
    "Subtract two images"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_subtract(image2.im, scale, offset))

def add_modulo(image1, image2):
    "Add two images without clipping"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_add_modulo(image2.im))

def subtract_modulo(image1, image2):
    "Subtract two images without clipping"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_subtract_modulo(image2.im))

def logical_and(image1, image2):
    "Logical and between two images"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_and(image2.im))

def logical_or(image1, image2):
    "Logical or between two images"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_or(image2.im))

def logical_xor(image1, image2):
    "Logical xor between two images"

    image1.load()
    image2.load()
    return image1._new(image1.im.chop_xor(image2.im))

def blend(image1, image2, alpha):
    "Blend two images using a constant transparency weight"

    return Image.blend(image1, image2, alpha)

def composite(image1, image2, mask):
    "Create composite image by blending images using a transparency mask"

    return Image.composite(image1, image2, mask)

def offset(image, xoffset, yoffset=None):
    "Offset image in horizontal and/or vertical direction"
    if yoffset is None:
        yoffset = xoffset
    image.load()
    return image._new(image.im.offset(xoffset, yoffset))
