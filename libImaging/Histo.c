/*
 * The Python Imaging Library
 * $Id: //modules/pil/libImaging/Histo.c#2 $
 *
 * histogram support
 *
 * history:
 * 95-06-15 fl	Created.
 * 96-04-05 fl	Fixed histogram for multiband images.
 * 97-02-23 fl	Added mask support
 * 98-07-01 fl	Added basic 32-bit float/integer support
 *
 * Copyright (c) Secret Labs AB 1997-98.
 * Copyright (c) Fredrik Lundh 1995-97.
 *
 * See the README file for information on usage and redistribution.
 */


#include "Imaging.h"


/* HISTOGRAM */
/* --------------------------------------------------------------------
 * Take a histogram of an image. Returns a histogram object containing
 * 256 slots per band in the input image.
 */

void
ImagingHistogramDelete(ImagingHistogram h)
{
    if (h->histogram)
	free(h->histogram);
    free(h);
}

ImagingHistogram
ImagingHistogramNew(Imaging im)
{
    ImagingHistogram h;

    /* Create histogram descriptor */
    h = calloc(1, sizeof(struct ImagingHistogramInstance));
    strcpy(h->mode, im->mode);
    h->bands = im->bands;
    h->histogram = calloc(im->pixelsize, 256 * sizeof(long));

    return h;
}

ImagingHistogram
ImagingGetHistogram(Imaging im, Imaging imMask, void* minmax)
{
    int x, y, i;
    ImagingHistogram h;
    INT32 imin, imax;
    FLOAT32 fmin, fmax, scale;

    if (!im)
	return ImagingError_ModeError();

    if (imMask) {
	/* Validate mask */
	if (im->xsize != imMask->xsize || im->ysize != imMask->ysize)
	    return ImagingError_Mismatch();
	if (strcmp(imMask->mode, "1") != 0 && strcmp(imMask->mode, "L") != 0)
	    return ImagingError_ValueError("bad transparency mask");
    }

    h = ImagingHistogramNew(im);

    if (imMask) {
	/* mask */
	if (im->image8) {
	    for (y = 0; y < im->ysize; y++)
		for (x = 0; x < im->xsize; x++)
		    if (imMask->image8[y][x] != 0)
			h->histogram[im->image8[y][x]]++;
	} else { /* yes, we need the braces. C isn't Python! */
            if (im->type != IMAGING_TYPE_UINT8)
                return ImagingError_ModeError();
	    for (y = 0; y < im->ysize; y++) {
		UINT8* in = (UINT8*) im->image32[y];
		for (x = 0; x < im->xsize; x++)
		    if (imMask->image8[y][x] != 0) {
			h->histogram[(*in++)]++;
			h->histogram[(*in++)+256]++;
			h->histogram[(*in++)+512]++;
			h->histogram[(*in++)+768]++;
		    } else
			in += 4;
	    }
	}
    } else {
	/* mask not given; process pixels in image */
	if (im->image8)
	    for (y = 0; y < im->ysize; y++)
		for (x = 0; x < im->xsize; x++)
		    h->histogram[im->image8[y][x]]++;
	else {
            switch (im->type) {
            case IMAGING_TYPE_UINT8:
                for (y = 0; y < im->ysize; y++) {
                    UINT8* in = (UINT8*) im->image[y];
                    for (x = 0; x < im->xsize; x++) {
                        h->histogram[(*in++)]++;
                        h->histogram[(*in++)+256]++;
                        h->histogram[(*in++)+512]++;
                        h->histogram[(*in++)+768]++;
                    }
                }
                break;
            case IMAGING_TYPE_INT32:
                if (!minmax)
                    return ImagingError_ValueError("min/max not given");
                if (!im->xsize || !im->ysize)
                    break;
                imin = ((INT32*) minmax)[0];
                imax = ((INT32*) minmax)[1];
                if (imin >= imax)
                    break;
                scale = 255.0 / (imax - imin);
                for (y = 0; y < im->ysize; y++) {
                    INT32* in = im->image32[y];
                    for (x = 0; x < im->xsize; x++) {
                        i = (int) (((*in++)-imin)*scale);
                        if (i >= 0 && i < 256)
                            h->histogram[i]++;
                    }
                }
                break;
            case IMAGING_TYPE_FLOAT32:
                if (!minmax)
                    return ImagingError_ValueError("min/max not given");
                if (!im->xsize || !im->ysize)
                    break;
                fmin = ((FLOAT32*) minmax)[0];
                fmax = ((FLOAT32*) minmax)[1];
                if (fmin >= fmax)
                    break;
                scale = 255.0 / (fmax - fmin);
                for (y = 0; y < im->ysize; y++) {
                    FLOAT32* in = (FLOAT32*) im->image32[y];
                    for (x = 0; x < im->xsize; x++) {
                        i = (int) (((*in++)-fmin)*scale);
                        if (i >= 0 && i < 256)
                            h->histogram[i]++;
                    }
                }
                break;
            }
        }
    }

    return h;
}
