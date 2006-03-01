/*
 * The Python Imaging Library.
 * $Id: //modules/pil/libImaging/coretest.c#2 $
 *
 * minimal test hack for the core library
 *
 * history:
 *	95-11-26 fl:	Created
 *
 * Copyright (c) Fredrik Lundh 1995.
 * Copyright (c) Secret Labs AB 1997.
 *
 * See the README file for information on usage and redistribution.
 */


#include <time.h>	/* clock() */

#include "Imaging.h"

#ifdef macintosh
#define TESTFILE "::Images:lena.ppm"
#else
#define TESTFILE "../Images/lena.ppm"
#endif /* macintosh */


int
main()
{
    Imaging im, im1, im2, im3;
    time_t t;

    /* Configuration sanity checks */

    if (sizeof(UINT8) != 1)
	printf("*** ERROR! Wrong type chosen for 8-bit pixels...\n");

    if (sizeof(INT32) != 4)
	printf("*** ERROR! Wrong type chosen for 32-bit pixels...\n");

    t = clock();

    /* Library test */

    im = ImagingOpenPPM(TESTFILE);

    im1 = ImagingFillRadialGradient("L");
    im3 = ImagingNew(im1->mode, im->xsize, im->ysize);
    ImagingResize(im3, im1, IMAGING_TRANSFORM_NEAREST);
    ImagingDelete(im1);
    ImagingSavePPM(im3, "wedge.ppm");

    if (im) {
	/* Just rush through some library code and see what happens */
	printf(">>> processing lena.ppm...\n");
	im3 = ImagingCrop(im3, 5, 5, im->xsize-5, im->ysize-5);
	im2 = ImagingCrop(im, 5, 5, im->xsize-5, im->ysize-5);
	im1 = ImagingNegative(im2);
        ImagingDelete(im2); im2 = im1;
	ImagingPaste(im, im2, im3, 5, 5, im->xsize-5, im->ysize-5);
	ImagingDelete(im3);
	ImagingSavePPM(im, "test.ppm");
    }

    printf(">>> elapsed time: %ld\n", (long) (clock() - t));

    ImagingDelete(im);

    printf(">>> as far as we tested, everything seems to be ok...\n");

    return 0;
}                                         
