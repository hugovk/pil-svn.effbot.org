/*
 * The Python Imaging Library
 * $Id: //modules/pil/libImaging/Antialias.c#6 $
 *
 * pilopen antialiasing support 
 *
 * history:
 * 2002-03-09 fl  Created (for PIL 1.1.3)
 * 2002-03-10 fl  Added support for mode "F"
 *
 * Copyright (c) 1997-2002 by Secret Labs AB
 *
 * See the README file for information on usage and redistribution.
 */

#include "Imaging.h"

#include <math.h>

/* resampling filters (from antialias.py) */

struct filter {
    float (*filter)(float x);
    float support;
};

static float sinc(float x)
{
    if (x == 0.0)
        return 1.0;
    x = x * M_PI;
    return sin(x) / x;
}

static float antialias(float x)
{
    /* lanczos (truncated sinc) */
    if (-3.0 <= x && x < 3.0)
        return sinc(x) * sinc(x/3);
    return 0.0;
}

static struct filter ANTIALIAS = { antialias, 3.0 };

static float nearest(float x)
{
    if (-0.5 <= x && x < 0.5)
        return 1.0;
    return 0.0;
}

static struct filter NEAREST = { nearest, 0.5 };

static float bilinear(float x)
{
    if (x < 0.0)
        x = -x;
    if (x < 1.0)
        return 1.0-x;
    return 0.0;
}

static struct filter BILINEAR = { bilinear, 1.0 };

static float bicubic(float x)
{
    /* FIXME: double-check this algorithm */
    /* FIXME: for best results, "a" should be -0.5 to -1.0, but we'll
       set it to zero for now, to match the 1.1 magnifying filter */
#define a 0.0
    if (x < 0.0)
        x = -x;
    if (x < 1.0)
        return (((a + 2.0) * x) - (a + 3.0)) * x*x + 1;
    if (x < 2.0)
        return (((a * x) - 5*a) * x + 8) * x - 4*a;
    return 0.0;
#undef a
}

static struct filter BICUBIC = { bicubic, 2.0 };

Imaging
ImagingStretch(Imaging imOut, Imaging imIn, int filter)
{
    /* FIXME: this is a quick and straightforward translation from a
       python prototype.  might need some further C-ification... */

    struct filter *filterp;
    float support, scale, filterscale;
    float center, ww, ss, ymin, ymax, xmin, xmax;
    int xx, yy, x, y, b;
    float *k;

    /* check modes */
    if (!imOut || !imIn || strcmp(imIn->mode, imOut->mode) != 0)
	return (Imaging) ImagingError_ModeError();

    /* check filter */
    switch (filter) {
    case IMAGING_TRANSFORM_NEAREST:
        filterp = &NEAREST;
        break;
    case IMAGING_TRANSFORM_ANTIALIAS:
        filterp = &ANTIALIAS;
        break;
    case IMAGING_TRANSFORM_BILINEAR:
        filterp = &BILINEAR;
        break;
    case IMAGING_TRANSFORM_BICUBIC:
        filterp = &BICUBIC;
        break;
    default:
        return (Imaging) ImagingError_ValueError(
            "unsupported resampling filter"
            );
    }

    if (imIn->ysize == imOut->ysize) {
        /* prepare for horizontal stretch */
        filterscale = scale = (float) imIn->xsize / imOut->xsize;
    } else if (imIn->xsize == imOut->xsize) {
        /* prepare for vertical stretch */
        filterscale = scale = (float) imIn->ysize / imOut->ysize;
    } else
	return (Imaging) ImagingError_Mismatch();

    /* determine support size (length of resampling filter) */
    support = filterp->support;

    if (filterscale < 1.0) {
        filterscale = 1.0;
        support = 0.5;
    }
    
    support = support * filterscale;

    /* coefficient buffer (with rounding safety margin) */
    k = malloc(((int) support * 2 + 10) * sizeof(float));
    if (!k)
        return (Imaging) ImagingError_MemoryError();

    if (imIn->xsize == imOut->xsize) {
        /* vertical stretch */
        for (yy = 0; yy < imOut->ysize; yy++) {
            center = yy * scale;
            ww = 0.0;
            ss = 1.0 / filterscale;
            /* calculate filter weights */
            ymin = floor(center - support);
            if (ymin < 0.0)
                ymin = 0.0;
            ymax = ceil(center + support);
            if (ymax > imIn->ysize)
                ymax = imIn->ysize;
            for (y = (int) ymin; y < (int) ymax; y++) {
                float w = filterp->filter((y - center + 0.5) * ss) * ss;
                k[y - (int) ymin] = w;
                ww = ww + w;
            }
            if (ww == 0.0)
                ww = 1.0;
            else
                ww = 1.0 / ww;
            if (imIn->image8) {
                /* 8-bit grayscale */
                for (xx = 0; xx < imOut->xsize; xx++) {
                    ss = 0.0;
                    for (y = (int) ymin; y < (int) ymax; y++)
                        ss = ss + imIn->image8[y][xx] * k[y - (int) ymin];
                    ss = ss * ww;
                    if (ss <= 0.0)
                        imOut->image[yy][xx] = 0;
                    else if (ss >= 255.0)
                        imOut->image[yy][xx] = 255;
                    else
                        imOut->image[yy][xx] = ss;
                }
            } else
                switch(imIn->type) {
                case IMAGING_TYPE_UINT8:
                    /* n-bit grayscale */
                    for (xx = 0; xx < imOut->xsize*4; xx++) {
                        /* FIXME: skip over unused pixels */
                        ss = 0.0;
                        for (y = (int) ymin; y < (int) ymax; y++)
                            ss = ss + (UINT8) imIn->image[y][xx] * k[y-(int) ymin];
                        ss = ss * ww;
                        if (ss <= 0.0)
                            imOut->image[yy][xx] = 0;
                        else if (ss >= 255.0)
                            imOut->image[yy][xx] = 255;
                        else
                            imOut->image[yy][xx] = ss;
                    }
                    break;
                case IMAGING_TYPE_INT32:
                    /* 32-bit integer */
                    for (xx = 0; xx < imOut->xsize; xx++) {
                        ss = 0.0;
                        for (y = (int) ymin; y < (int) ymax; y++)
                            ss = ss + imIn->image32[y][xx] * k[y - (int) ymin];
                        imOut->image32[yy][xx] = ss * ww;
                    }
                    break;
                case IMAGING_TYPE_FLOAT32:
                    /* 32-bit float */
                    for (xx = 0; xx < imOut->xsize; xx++) {
                        ss = 0.0;
                        for (y = (int) ymin; y < (int) ymax; y++)
                            ss = ss + ((FLOAT32*) imIn->image32[y])[xx] * k[y - (int) ymin];
                        ((FLOAT32*) imOut->image32[yy])[xx] = ss * ww;
                    }
                    break;
                default:
                    return (Imaging) ImagingError_ModeError();
                }
        }
    } else {
        /* horizontal stretch */
        for (xx = 0; xx < imOut->xsize; xx++) {
            center = xx * scale;
            ww = 0.0;
            ss = 1.0 / filterscale;
            xmin = floor(center - support);
            if (xmin < 0.0)
                xmin = 0.0;
            xmax = ceil(center + support);
            if (xmax > imIn->xsize)
                xmax = imIn->xsize;
            for (x = (int) xmin; x < (int) xmax; x++) {
                float w = filterp->filter((x - center + 0.5) * ss) * ss;
                k[x - (int) xmin] = w;
                ww = ww + w;
            }
            if (ww == 0.0)
                ww = 1.0;
            else
                ww = 1.0 / ww;
            if (imIn->image8) {
                /* 8-bit grayscale */
                for (yy = 0; yy < imOut->ysize; yy++) {
                    ss = 0.0;
                    for (x = (int) xmin; x < (int) xmax; x++)
                        ss = ss + imIn->image8[yy][x] * k[x - (int) xmin];
                    ss = ss * ww;
                    if (ss <= 0.0)
                        imOut->image[yy][xx] = 0;
                    else if (ss >= 255.0)
                        imOut->image[yy][xx] = 255;
                    else
                        imOut->image[yy][xx] = ss;
                }
            } else
                switch(imIn->type) {
                case IMAGING_TYPE_UINT8:
                    /* n-bit grayscale */
                    for (yy = 0; yy < imOut->ysize; yy++) {
                        for (b = 0; b < imIn->bands; b++) {
                            ss = 0.0;
                            for (x = (int) xmin; x < (int) xmax; x++)
                                ss = ss + (UINT8) imIn->image[yy][x*4+b] * k[x - (int) xmin];
                            ss = ss * ww;
                            if (ss <= 0.0)
                                imOut->image[yy][xx*4+b] = 0;
                            else if (ss >= 255.0)
                                imOut->image[yy][xx*4+b] = 255;
                            else
                                imOut->image[yy][xx*4+b] = ss;
                        }
                    }
                    break;
                case IMAGING_TYPE_INT32:
                    /* 32-bit integer */
                    for (yy = 0; yy < imOut->ysize; yy++) {
                        ss = 0.0;
                        for (x = (int) xmin; x < (int) xmax; x++)
                            ss = ss + imIn->image32[yy][x] * k[x - (int) xmin];
                        imOut->image32[yy][xx] = ss * ww;
                    }
                    break;
                case IMAGING_TYPE_FLOAT32:
                    /* 32-bit float */
                    for (yy = 0; yy < imOut->ysize; yy++) {
                        ss = 0.0;
                        for (x = (int) xmin; x < (int) xmax; x++)
                            ss = ss + ((FLOAT32*) imIn->image[yy])[x] * k[x - (int) xmin];
                        ((FLOAT32*) imOut->image[yy])[xx] = ss * ww;
                    }
                    break;
                default:
                    return (Imaging) ImagingError_ModeError();
                }
        }
    }

    free(k);

    return imOut;
}
