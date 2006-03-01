#
# The Python Imaging Library.
# $Id: //modules/pil/PIL/ImageOps.py#1 $
#
# standard image operations
#
# History:
# 2001-10-20 fl   Created
# 2001-10-23 fl   Added autocontrast operator
# 2001-12-18 fl   Added Kevin's fit operator
#
# Copyright (c) 2001 by Secret Labs AB
# Copyright (c) 2001 by Fredrik Lundh
#
# See the README file for information on usage and redistribution.
#

import Image

#
# helpers

def _border(border):
    if type(border) is type(()):
        if len(border) == 2:
            left, top = right, bottom = border
        elif len(border) == 4:
            left, top, right, bottom = border
    else:
        left = top = right = bottom = border
    return left, top, right, bottom

def _lut(image, lut):
    if image.mode == "P":
        # FIXME: apply to lookup table, not image data
        raise NotImplemented, "mode P support coming soon"
    elif image.mode in ("L", "RGB"):
        if image.mode == "RGB" and len(lut) == 256:
            lut = lut + lut + lut
        return image.point(lut)
    else:
        raise IOError, "not supported for this image mode"

#
# actions

def autocontrast(image, cutoff=0, ignore=None):
    "Maximize image contrast, based on histogram"
    histogram = image.histogram()
    lut = []
    for layer in range(0, len(histogram), 256):
        h = histogram[layer:layer+256]
        if ignore is not None:
            # get rid of outliers
            try:
                h[ignore] = 0
            except TypeError:
                # assume sequence
                for ix in ignore:
                    h[ix] = 0
        if cutoff:
            # cut off pixels from both ends of the histogram
            # get number of pixels
            n = 0
            for ix in range(256):
                n = n + h[ix]
            # remove cutoff% pixels from the low end
            cut = n * cutoff / 100
            for lo in range(256):
                if cut > h[lo]:
                    cut = cut - h[lo]
                    h[lo] = 0
                else:
                    h[lo] = h[lo] - cut
                    cut = 0
                if cut <= 0:
                    break
            # remove cutoff% samples from the hi end
            cut = n * cutoff / 100
            for hi in range(255, -1, -1):
                if cut > h[hi]:
                    cut = cut - h[hi]
                    h[hi] = 0
                else:
                    h[hi] = h[hi] - cut
                    cut = 0
                if cut <= 0:
                    break
        # find lowest/highest samples after preprocessing
        for lo in range(256):
            if h[lo]:
                break
        for hi in range(255, -1, -1):
            if h[hi]:
                break
        if hi <= lo:
            # don't bother
            lut.extend(range(256))
        else:
            scale = 255.0 / (hi - lo)
            offset = -lo * scale
            for ix in range(256):
                ix = int(ix * scale + offset)
                if ix < 0:
                    ix = 0
                elif ix > 255:
                    ix = 255
                lut.append(ix)
    return _lut(image, lut)

def colorize(image, black, white):
    "Colorize a grayscale image"
    assert image.mode == "L"
    red = []; green = []; blue = []
    for i in range(256):
        red.append(black[0]+i*(white[0]-black[0])/255)
        green.append(black[1]+i*(white[1]-black[1])/255)
        blue.append(black[2]+i*(white[2]-black[2])/255)
    image = image.convert("RGB")
    return _lut(image, red + green + blue)

def crop(image, border=0):
    "Crop border off image"
    left, top, right, bottom = _border(border)
    return image.crop(
        (left, top, image.size[0]-right, image.size[1]-bottom)
        )

def deform(image, deformer, resample=Image.BILINEAR):
    "Deform image using the given deformer"
    return image.transform(
        image.size, MESH, deformer.getmesh(image), resample
        )

def equalize(image):
    "Equalize image histogram"
    if image.mode == "P":
        h = image.convert("RGB").histogram()
    else:
        h = image.histogram()
    lut = []
    for b in range(0, len(h), 256):
        step = reduce(operator.add, h[b:b+256]) / 255
        n = 0
        for i in range(256):
            lut.append(n / step)
            n = n + h[i+b]
    return _lut(image, lut)

def expand(image, border=0, fill=0):
    "Add border to image"
    left, top, right, bottom = _border(border)
    width = left + image.size[0] + right
    height = top + image.size[1] + buttom
    out = Image.new(image.mode, (width, height), fill)
    out.paste((left, top), image)
    return out

def fit(image, size, method=Image.NEAREST, bleed=0.0, centering=(0.5, 0.5)):
    """
    This method returns a sized and cropped version of the image,
    cropped to the aspect ratio and size that you request.  It can be
    customized to your needs with "method", "bleed", and "centering"
    as required.

    image: a PIL Image object

    size: output size in pixels, given as a (width, height) tuple

    method: resampling method

    bleed: decimal percentage (0-0.49999) of width/height to crop off
    as a minumum around the outside of the image This allows you to
    remove a default amount of the image around the outside, for
    'cleaning up' scans that may have edges of negs showing, or
    whatever.  This percentage is removed from EACH side, not a total
    amount.

    centering: (left, top), percentages to crop of each side to fit
    the aspect ratio you require.

    This function allows you to customize where the crop occurs,
    whether it is a 'center crop' or a 'top crop', or whatever.
    Default is center-cropped.

    (0.5, 0.5) is center cropping (i.e. if cropping the width, take
    50% off of the left side (and therefore 50% off the right side),
    and same with Top/Bottom)

    (0.0, 0.0) will crop from the top left corner (i.e. if cropping
    the width, take all of the crop off of the right side, and if
    cropping the height, take all of it off the bottom)

    (1.0, 0.0) will crop from the bottom left corner, etc. (i.e. if
    cropping the width, take all of the crop off the left side, and if
    cropping the height take none from the Top (and therefore all off
    the bottom))

    by Kevin Cazabon, Feb 17/2000
    kevin@cazabon.com
    http://www.cazabon.com
    """

    # ensure inputs are valid
    if type(centering) != type([]):
        centering = [centering[0], centering[1]]

    if centering[0] > 1.0 or centering[0] < 0.0:
        centering [0] = 0.50
    if centering[1] > 1.0 or centering[1] < 0.0:
        centering[1] = 0.50

    if bleed > 0.49999 or bleed < 0.0:
        bleed = 0.0

    # calculate the area to use for resizing and cropping, subtracting
    # the 'bleed' around the edges

    # number of pixels to trim off on Top and Bottom, Left and Right
    bleedPixels = (
        int((float(bleed) * float(image.size[0])) + 0.5),
        int((float(bleed) * float(image.size[1])) + 0.5)
        )

    liveArea = (
        bleedPixels[0], bleedPixels[1], image.size[0] - bleedPixels[0] - 1,
        image.size[1] - bleedPixels[1] - 1
        )

    liveSize = (liveArea[2] - liveArea[0], liveArea[3] - liveArea[1])

    # calculate the aspect ratio of the liveArea
    liveAreaAspectRatio = float(liveSize[0])/float(liveSize[1])

    # calculate the aspect ratio of the output image
    aspectRatio = float(size[0]) / float(size[1])

    # figure out if the sides or top/bottom will be cropped off
    if liveAreaAspectRatio >= aspectRatio:
        # liveArea is wider than what's needed, crop the sides
        cropWidth = int((aspectRatio * float(liveSize[1])) + 0.5)
        cropHeight = liveSize[1]
    else:
        # liveArea is taller than what's needed, crop the top and bottom
        cropWidth = liveSize[0]
        cropHeight = int((float(liveSize[0])/aspectRatio) + 0.5)

    # make the crop
    leftSide = int(liveArea[0] + (float(liveSize[0]-cropWidth) * centering[0]))
    if leftSide < 0:
        leftSide = 0
    topSide = int(liveArea[1] + (float(liveSize[1]-cropHeight) * centering[1]))
    if topSide < 0:
        topSide = 0

    out = image.crop(
        (leftSide, topSide, leftSide + cropWidth, topSide + cropHeight)
        )

    # resize the image and return it
    return out.resize(size, method)

def flip(image):
    "Flip image vertically"
    return image.transpose(Image.FLIP_TOP_BOTTOM)

def grayscale(image):
    "Convert to grayscale"
    return image.convert("L")

def invert(image):
    "Invert image (negative)"
    lut = []
    for i in range(256):
        lut.append(255-i)
    return _lut(image, lut)

def mirror(image):
    "Flip image horizontally"
    return image.transpose(Image.FLIP_LEFT_RIGHT)

def posterize(image, bits):
    "Reduce the number of bits per color channel"
    lut = []
    mask = ~(2**(8-bits)-1)
    for i in range(256):
        lut.append(i & mask)
    return _lut(image, lut)

def solarize(image, threshold=128):
    "Invert all values above threshold"
    lut = []
    for i in range(256):
        if i < threshold:
            lut.append(i)
        else:
            lut.append(255-i)
    return _lut(image, lut)
