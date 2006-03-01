#
# The Python Imaging Library.
# $Id$
#
# image enhancement classes
#
# For a background, see "Image Processing By Interpolation and
# Extrapolation", Paul Haeberli and Douglas Voorhies.  Available
# at http://www.sgi.com/grafica/interp/index.html
#
# History:
#       96-03-23 fl     Created
#
# Copyright (c) Secret Labs AB 1997.
# Copyright (c) Fredrik Lundh 1996.
#
# See the README file for information on usage and redistribution.
#

import Image, ImageFilter

class _Enhance:
    def enhance(self, alpha):
        return Image.blend(self.degenerate, self.image, alpha)

class Color(_Enhance):
    "Adjust image colour balance"
    def __init__(self, image):
        self.image = image
        self.degenerate = image.convert("L").convert(image.mode)

class Contrast(_Enhance):
    "Adjust image contrast"
    def __init__(self, image):
        self.image = image
        mean = reduce(lambda a,b: a+b, image.convert("L").histogram())/256.0
        self.degenerate = Image.new("L", image.size, mean).convert(image.mode)

class Brightness(_Enhance):
    "Adjust image brightness"
    def __init__(self, image):
        self.image = image
        self.degenerate = Image.new(image.mode, image.size, 0)

class Sharpness(_Enhance):
    "Adjust image sharpness"
    def __init__(self, image):
        self.image = image
        self.degenerate = image.filter(ImageFilter.SMOOTH)
