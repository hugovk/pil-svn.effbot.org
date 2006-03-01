/*
 * The Python Imaging Library
 * $Id: //modules/pil/libImaging/Point.c#2 $
 *
 * point (pixel) translation
 *
 * history:
 *	95-11-27 fl:	Created
 *	96-03-31 fl:	Fixed colour support
 *	96-08-13 fl:	Support 8-bit to "1" thresholding
 *	97-05-31 fl:	Added floating point transform
 *	98-07-02 fl:	Added integer point transform
 *	98-07-17 fl:	Support 8-bit to anything lookup
 *
 * Copyright (c) Secret Labs AB 1997-98.
 * Copyright (c) Fredrik Lundh 1995-97.
 *
 * See the README file for information on usage and redistribution.
 */


#include "Imaging.h"


Imaging
ImagingPoint(Imaging imIn, const char* mode, const void* table_)
{
    /* lookup table transform */

    Imaging imOut;
    int x, y;

    if (!imIn)
	return (Imaging) ImagingError_ModeError();

    if (mode && !imIn->image8)
	return (Imaging) ImagingError_Mismatch();

    imOut = ImagingNew((mode) ? mode : imIn->mode, imIn->xsize, imIn->ysize);
    if (!imOut)
	return NULL;

    ImagingCopyInfo(imOut, imIn);

    if (imIn->image8) {

        if (imOut->image8) {

            /* 8-bit source, 8-bit destination */
            UINT8* table = (UINT8*) table_;
            for (y = 0; y < imIn->ysize; y++) {
                UINT8* in = imIn->image8[y];
                UINT8* out = imOut->image8[y];
                for (x = 0; x < imIn->xsize; x++)
                    imOut->image[y][x] = table[imIn->image8[y][x]];
            }

        } else {

            /* 8-bit source, 32-bit destination */
            INT32* table = (INT32*) table_;
            for (y = 0; y < imIn->ysize; y++) {
                UINT8* in = imIn->image8[y];
                INT32* out = imOut->image32[y];
                for (x = 0; x < imIn->xsize; x++)
                    out[x] = table[in[x]];
            }

        }

    } else {

        /* nx8-bit source, nx8-bit destination */
        UINT8* table = (UINT8*) table_;
        switch (imIn->bands) {
        case 3:
            for (y = 0; y < imIn->ysize; y++) {
                UINT8* in = (UINT8*) imIn->image[y];
                UINT8* out = (UINT8*) imOut->image[y];
                for (x = 0; x < imIn->xsize; x++) {
                    out[0] = table[in[0]];
                    out[1] = table[in[1]+256];
                    out[2] = table[in[2]+512];
                    in += 4; out += 4;
                }
            }
            break;
        case 4:
            for (y = 0; y < imIn->ysize; y++) {
                UINT8* in = (UINT8*) imIn->image[y];
                UINT8* out = (UINT8*) imOut->image[y];
                for (x = 0; x < imIn->xsize; x++) {
                    out[0] = table[in[0]];
                    out[1] = table[in[1]+256];
                    out[2] = table[in[2]+512];
                    out[3] = table[in[3]+768];
                    in += 4; out += 4;
                }
            }
            break;
        }
    }

    return imOut;
}


Imaging
ImagingPointTransform(Imaging imIn, double scale, double offset)
{
    /* scale/offset transform */

    Imaging imOut;
    int x, y;

    if (!imIn || strcmp(imIn->mode, "I") != 0 && strcmp(imIn->mode, "F") != 0)
	return (Imaging) ImagingError_ModeError();

    imOut = ImagingNew(imIn->mode, imIn->xsize, imIn->ysize);
    if (!imOut)
	return NULL;

    ImagingCopyInfo(imOut, imIn);

    switch (imIn->type) {
    case IMAGING_TYPE_INT32:
        for (y = 0; y < imIn->ysize; y++) {
            INT32* in  = imIn->image32[y];
            INT32* out = imOut->image32[y];
            /* FIXME: add clipping? */
            for (x = 0; x < imIn->xsize; x++)
                out[x] = in[x] * scale + offset;
        }
        break;
    case IMAGING_TYPE_FLOAT32:
        for (y = 0; y < imIn->ysize; y++) {
            FLOAT32* in  = (FLOAT32*) imIn->image32[y];
            FLOAT32* out = (FLOAT32*) imOut->image32[y];
            for (x = 0; x < imIn->xsize; x++)
                out[x] = in[x] * scale + offset;
        }
        break;
    default:
        ImagingDelete(imOut);
        return (Imaging) ImagingError_ValueError("internal error");
    }

    return imOut;
}
