/*
 * The Python Imaging Library
 * $Id$
 *
 * apply convolution kernel to image
 *
 * history:
 * 95-11-26 fl	Created, supports 3x3 kernels
 * 95-11-27 fl	Added 5x5 kernels, copy border
 * 99-07-26 fl	Eliminated a few compiler warnings
 *
 * Copyright (c) Secret Labs AB 1997-99.  All rights reserved.
 * Copyright (c) Fredrik Lundh 1995.
 *
 * See the README file for information on usage and redistribution.
 */

/*
 * FIXME: Support RGB and RGBA/CMYK modes as well
 * FIXME: Expand image border (current version leaves border as is)
 * FIXME: Implement image processing gradient filters
 */

#include "Imaging.h"


/* Image enhancement filters. This set was snatched from LVIEW Pro. */

int _ImagingFilterSmooth[] = {
    3, 3, 13, 0,
     1,  1,  1,
     1,  5,  1,
     1,  1,  1
    };

Imaging ImagingFilterSmooth(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterSmooth);
}

int _ImagingFilterSmoothMore[] = {
    5, 5, 100, 0,
     1,  1,  1,  1,  1,
     1,  5,  5,  5,  1,
     1,  5, 44,  5,  1,
     1,  5,  5,  5,  1,
     1,  1,  1,  1,  1
    };

Imaging ImagingFilterSmoothMore(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterSmoothMore);
}

int _ImagingFilterBlur[] = {
    5, 5, 16, 0,
     1,  1,  1,  1,  1,
     1,  0,  0,  0,  1,
     1,  0,  0,  0,  1,
     1,  0,  0,  0,  1,
     1,  1,  1,  1,  1
    };

Imaging ImagingFilterBlur(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterBlur);
}

int _ImagingFilterSharpen[] = {
    3, 3, 16, 0,
    -2, -2, -2,
    -2, 32, -2,
    -2, -2, -2
    };

Imaging ImagingFilterSharpen(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterSharpen);
}

int _ImagingFilterDetail[] = {
    3, 3, 6, 0,
     0, -1,  0,
    -1, 10, -1,
     0, -1,  0
    };

Imaging ImagingFilterDetail(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterDetail);
}

int _ImagingFilterEdgeEnhance[] = {
    3, 3, 2, 0,
    -1, -1, -1,
    -1, 10, -1,
    -1, -1, -1
    };

Imaging ImagingFilterEdgeEnhance(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterEdgeEnhance);
}

int _ImagingFilterEdgeEnhanceMore[] = {
    3, 3, 1, 0,
    -1, -1, -1,
    -1,  9, -1,
    -1, -1, -1
    };

Imaging ImagingFilterEdgeEnhanceMore(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterEdgeEnhanceMore);
}

int _ImagingFilterFindEdges[] = {
    3, 3, 1, 0,
    -1, -1, -1,
    -1,  8, -1,
    -1, -1, -1
    };

Imaging ImagingFilterFindEdges(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterFindEdges);
}

int _ImagingFilterContour[] = {
    3, 3, 1, 255,
    -1, -1, -1,
    -1,  8, -1,
    -1, -1, -1
    };

Imaging ImagingFilterContour(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterContour);
}

/* Special effects filters */

int _ImagingFilterEmboss[] = {
    3, 3, 1, 128,
    -1,  0,  0,
     0,  1,  0,
     0,  0,  0
    };

Imaging ImagingFilterEmboss(Imaging im)
{
    return ImagingFilter(im, _ImagingFilterEmboss);
}


Imaging
ImagingFilter(Imaging im, const int* kernel)
{
    Imaging imOut;
    int x, y;
    int xkernel, ykernel;
    int sum, divisor, offset;

    if (!im || strcmp(im->mode, "L") != 0)
	return (Imaging) ImagingError_ModeError();

    xkernel = kernel[0];
    ykernel = kernel[1];

    if ((xkernel != 3 && xkernel != 5) || xkernel != ykernel)
	return (Imaging) ImagingError_ValueError("bad kernel size");

    divisor = kernel[2];
    offset  = kernel[3];

    kernel += 4;

    imOut = ImagingNew(im->mode, im->xsize, im->ysize);
    if (!imOut)
	return NULL;

    /* brute force kernel implementations */
#define	KERNEL3x3(image, kernel, d) ( \
    (int) image[y+1][x-d] * kernel[0] + \
    (int) image[y+1][x]   * kernel[1] + \
    (int) image[y+1][x+d] * kernel[2] + \
    (int) image[y][x-d]   * kernel[3] + \
    (int) image[y][x]     * kernel[4] + \
    (int) image[y][x+d]   * kernel[5] + \
    (int) image[y-1][x-d] * kernel[6] + \
    (int) image[y-1][x]   * kernel[7] + \
    (int) image[y-1][x+d] * kernel[8])

#define	KERNEL5x5(image, kernel, d) ( \
    (int) image[y+2][x-d-d] * kernel[0] + \
    (int) image[y+2][x-d]   * kernel[1] + \
    (int) image[y+2][x]     * kernel[2] + \
    (int) image[y+2][x+d]   * kernel[3] + \
    (int) image[y+2][x+d+d] * kernel[4] + \
    (int) image[y+1][x-d-d] * kernel[5] + \
    (int) image[y+1][x-d]   * kernel[6] + \
    (int) image[y+1][x]     * kernel[7] + \
    (int) image[y+1][x+d]   * kernel[8] + \
    (int) image[y+1][x+d+d] * kernel[9] + \
    (int) image[y][x-d-d]   * kernel[10] + \
    (int) image[y][x-d]     * kernel[11] + \
    (int) image[y][x]       * kernel[12] + \
    (int) image[y][x+d]     * kernel[13] + \
    (int) image[y][x+d+d]   * kernel[14] + \
    (int) image[y-1][x-d-d] * kernel[15] + \
    (int) image[y-1][x-d]   * kernel[16] + \
    (int) image[y-1][x]     * kernel[17] + \
    (int) image[y-1][x+d]   * kernel[18] + \
    (int) image[y-1][x+d+d] * kernel[19] + \
    (int) image[y-2][x-d-d] * kernel[20] + \
    (int) image[y-2][x-d]   * kernel[21] + \
    (int) image[y-2][x]     * kernel[22] + \
    (int) image[y-2][x+d]   * kernel[23] + \
    (int) image[y-2][x+d+d] * kernel[24])

    if (xkernel == 3) {
	/* 3x3 kernel. */
	for (x = 0; x < im->xsize; x++)
	    imOut->image[0][x] = im->image8[0][x];
	for (y = 1; y < im->ysize-1; y++) {
	    imOut->image[y][0] = im->image8[y][0];
	    for (x = 1; x < im->xsize-1; x++) {
		sum = KERNEL3x3(im->image8, kernel, 1) / divisor + offset;
		if (sum <= 0)
		    imOut->image8[y][x] = 0;
		else if (sum >= 255)
		    imOut->image8[y][x] = 255;
		else
		    imOut->image8[y][x] = sum;
	     }
	    imOut->image8[y][x] = im->image8[y][x];
	}
	for (x = 0; x < im->xsize; x++)
	    imOut->image8[y][x] = im->image8[y][x];
    } else {
	/* 5x5 kernel. */
	for (y = 0; y < 2; y++)
	    for (x = 0; x < im->xsize; x++)
		imOut->image8[y][x] = im->image8[y][x];
	for (; y < im->ysize-2; y++) {
	    for (x = 0; x < 2; x++)
		imOut->image8[y][x] = im->image8[y][x];
	    for (; x < im->xsize-2; x++) {
		sum = KERNEL5x5(im->image8, kernel, 1) / divisor + offset;
		if (sum <= 0)
		    imOut->image8[y][x] = 0;
		else if (sum >= 255)
		    imOut->image8[y][x] = 255;
		else
		    imOut->image8[y][x] = sum;
	    }
	    for (; x < im->xsize; x++)
		imOut->image8[y][x] = im->image8[y][x];
	}
	for (; y < im->ysize; y++)
	    for (x = 0; x < im->xsize; x++)
		imOut->image8[y][x] = im->image8[y][x];
    }
    return imOut;
}

