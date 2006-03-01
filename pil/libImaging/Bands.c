/* 
 * The Python Imaging Library
 * $Id: //modules/pil/libImaging/Bands.c#2 $
 * 
 * stuff to extract and paste back individual bands
 *
 * history:
 *	96-03-20 fl:	Created
 *	97-08-27 fl:	Fixed putband for single band targets.
 *
 * Copyright (c) Fredrik Lundh 1996-97.
 * Copyright (c) Secret Labs AB 1997.
 *
 * See the README file for details on usage and redistribution.
 */


#include "Imaging.h"


Imaging
ImagingGetBand(Imaging imIn, int band)
{
    Imaging imOut;
    int x, y;

    /* Check arguments */
    if (!imIn || imIn->type != IMAGING_TYPE_UINT8)
	return (Imaging) ImagingError_ModeError();

    if (band < 0 || band >= imIn->bands)
	return (Imaging) ImagingError_ValueError("band index out of range");

    /* Shortcuts */
    if (imIn->bands == 1)
	return ImagingCopy(imIn);

    imOut = ImagingNew("L", imIn->xsize, imIn->ysize);
    if (!imOut)
	return NULL;

    /* Extract band from image */
    for (y = 0; y < imIn->ysize; y++) {
	UINT8* in = (UINT8*) imIn->image[y] + band;
	UINT8* out = imOut->image8[y];
	for (x = 0; x < imIn->xsize; x++) {
	    out[x] = *in;
	    in += 4;
	}
    }

    return imOut;
}

Imaging
ImagingPutBand(Imaging imOut, Imaging imIn, int band)
{
    int x, y;

    /* Check arguments */
    if (!imIn || imIn->bands != 1 || !imOut)
	return (Imaging) ImagingError_ModeError();

    if (band < 0 || band >= imOut->bands)
	return (Imaging) ImagingError_ValueError("band index out of range");

    if (imIn->type  != imOut->type  ||
	imIn->xsize != imOut->xsize ||
	imIn->ysize != imOut->ysize)
	return (Imaging) ImagingError_Mismatch();

    /* Shortcuts */
    if (imOut->bands == 1)
	return ImagingCopy2(imOut, imIn);

    /* Insert band into image */
    for (y = 0; y < imIn->ysize; y++) {
	UINT8* in = imIn->image8[y];
	UINT8* out = (UINT8*) imOut->image[y] + band;
	for (x = 0; x < imIn->xsize; x++) {
	    *out = in[x];
	    out += 4;
	}
    }

    return imOut;
}
