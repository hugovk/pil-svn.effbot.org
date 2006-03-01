/* 
 * The Python Imaging Library
 * $Id: //modules/pil/libImaging/GetBBox.c#4 $
 *
 * get bounding box for image
 *
 * history:
 * 1996-07-22 fl   Created
 * 1996-12-30 fl   Added projection stuff
 * 1998-07-12 fl   Added extrema stuff
 *
 * Copyright (c) 1997-2003 by Secret Labs AB.
 * Copyright (c) 1996-2003 by Fredrik Lundh.
 *
 * See the README file for details on usage and redistribution.
 */


#include "Imaging.h"


int
ImagingGetBBox(Imaging im, int bbox[4])
{
    /* Get the bounding box for any non-zero data in the image.*/

    int x, y;
    int has_data;

    /* Initialize bounding box to max values */
    bbox[0] = im->xsize;
    bbox[1] = -1;
    bbox[2] = bbox[3] = 0;

#define	GETBBOX(image, mask)\
    for (y = 0; y < im->ysize; y++) {\
	has_data = 0;\
	for (x = 0; x < im->xsize; x++)\
	    if (im->image[y][x] & mask) {\
		has_data = 1;\
		if (x < bbox[0])\
		    bbox[0] = x;\
		if (x >= bbox[2])\
		    bbox[2] = x+1;\
	    }\
	if (has_data) {\
	    if (bbox[1] < 0)\
		bbox[1] = y;\
	    bbox[3] = y+1;\
	}\
    }

    if (im->image8) {
	GETBBOX(image8, 0xff);
    } else {
	INT32 mask = 0xffffffff;
	if (im->bands == 3)
	    ((UINT8*) &mask)[3] = 0;
	GETBBOX(image32, mask);
    }

    /* Check that we got a box */
    if (bbox[1] < 0)
	return 0; /* no data */

    return 1; /* ok */
}


int
ImagingGetProjection(Imaging im, UINT8* xproj, UINT8* yproj)
{
    /* Get projection arrays for non-zero data in the image.*/

    int x, y;
    int has_data;

    /* Initialize projection arrays */
    memset(xproj, 0, im->xsize);
    memset(yproj, 0, im->ysize);

#define	GETPROJ(image, mask)\
    for (y = 0; y < im->ysize; y++) {\
	has_data = 0;\
	for (x = 0; x < im->xsize; x++)\
	    if (im->image[y][x] & mask) {\
		has_data = 1;\
		xproj[x] = 1;\
	    }\
	if (has_data)\
	    yproj[y] = 1;\
    }

    if (im->image8) {
	GETPROJ(image8, 0xff);
    } else {
	INT32 mask = 0xffffffff;
	if (im->bands == 3)
	    ((UINT8*) &mask)[3] = 0;
	GETPROJ(image32, mask);
    }

    return 1; /* ok */
}



int
ImagingGetExtrema(Imaging im, void *extrema)
{
    int x, y;
    INT32 imin, imax;
    FLOAT32 fmin, fmax;

    if (im->bands != 1) {
        ImagingError_ModeError();
        return -1; /* mismatch */
    }

    if (!im->xsize || !im->ysize)
        return 0; /* zero size */

    switch (im->type) {
    case IMAGING_TYPE_UINT8:
        imin = imax = im->image8[0][0];
        for (y = 0; y < im->ysize; y++) {
            UINT8* in = im->image8[y];
            for (x = 0; x < im->xsize; x++) {
                if (imin > in[x])
                    imin = in[x];
                else if (imax < in[x])
                    imax = in[x];
            }
        }
        ((UINT8*) extrema)[0] = (UINT8) imin;
        ((UINT8*) extrema)[1] = (UINT8) imax;
        break;
    case IMAGING_TYPE_INT32:
        imin = imax = im->image32[0][0];
        for (y = 0; y < im->ysize; y++) {
            INT32* in = im->image32[y];
            for (x = 0; x < im->xsize; x++) {
                if (imin > in[x])
                    imin = in[x];
                else if (imax < in[x])
                    imax = in[x];
            }
        }
        ((INT32*) extrema)[0] = imin;
        ((INT32*) extrema)[1] = imax;
        break;
    case IMAGING_TYPE_FLOAT32:
        fmin = fmax = ((FLOAT32*) im->image32[0])[0];
        for (y = 0; y < im->ysize; y++) {
            FLOAT32* in = (FLOAT32*) im->image32[y];
            for (x = 0; x < im->xsize; x++) {
                if (fmin > in[x])
                    fmin = in[x];
                else if (fmax < in[x])
                    fmax = in[x];
            }
        }
        ((FLOAT32*) extrema)[0] = fmin;
        ((FLOAT32*) extrema)[1] = fmax;
        break;
    case IMAGING_TYPE_SPECIAL:
      if (strcmp(im->mode, "I;16") == 0) {
          imin = imax = ((UINT16*) im->image8[0])[0];
          for (y = 0; y < im->ysize; y++) {
              UINT16* in = (UINT16 *) im->image[y];
              for (x = 0; x < im->xsize; x++) {
                  if (imin > in[x])
                      imin = in[x];
                  else if (imax < in[x])
                      imax = in[x];
              }
          }
          ((UINT16*) extrema)[0] = (UINT16) imin;
          ((UINT16*) extrema)[1] = (UINT16) imax;
	  break;
      }
      /* FALL THROUGH */
    default:
        ImagingError_ModeError();
        return -1;
    }
    return 1; /* ok */
}
