/*
 * The Python Imaging Library
 * $Id: //modules/pil/libImaging/Storage.c#2 $
 *
 * imaging storage object
 *
 * This baseline implementation is designed to efficiently handle
 * large images, provided they fit into the available memory.
 *
 * history:
 * 95-06-15 fl	Created
 * 95-09-12 fl	Updated API, compiles silently under ANSI C++
 * 95-11-26 fl	Compiles silently under Borland 4.5 as well
 * 96-05-05 fl	Correctly test status from Prologue
 * 97-05-12 fl	Increased THRESHOLD (to speed up Tk interface)
 * 97-05-30 fl	Added support for floating point images
 * 97-11-17 fl	Added support for "RGBX" images
 * 98-01-11 fl	Added support for integer images
 * 98-03-05 fl	Exported Prologue/Epilogue functions
 * 98-07-01 fl	Added basic "YCrCb" support
 * 98-07-03 fl	Attach palette in prologue for "P" images
 * 98-07-09 hk	Don't report MemoryError on zero-size images
 * 98-07-12 fl	Change "YCrCb" to "YCbCr" (!)
 * 98-10-26 fl	Added "I;16" and "I;16B" storage modes (experimental)
 * 98-12-29 fl	Fixed allocation bug caused by previous fix
 * 99-02-03 fl	Added "RGBa" and "BGR" modes (experimental)
 *
 * Copyright (c) Secret Labs AB 1998-99.
 * Copyright (c) Fredrik Lundh 1995-97.
 *
 * See the README file for information on usage and redistribution.
 */


#include "Imaging.h"


/* --------------------------------------------------------------------
 * Standard image object.
 */

Imaging
ImagingNewPrologue(const char *mode, unsigned xsize, unsigned ysize)
{
    Imaging im;

    im = (Imaging) calloc(1, sizeof(struct ImagingMemoryInstance));
    if (!im)
	return (Imaging) ImagingError_MemoryError();

    /* Setup image descriptor */
    im->xsize = xsize;
    im->ysize = ysize;

    im->type = IMAGING_TYPE_UINT8;

    if (strcmp(mode, "1") == 0) {
        /* 1-bit images */
        im->bands = im->pixelsize = 1;
        im->linesize = xsize;

    } else if (strcmp(mode, "P") == 0) {
        /* 8-bit palette mapped images */
        im->bands = im->pixelsize = 1;
        im->linesize = xsize;
        im->palette = ImagingPaletteNew("RGB");

    } else if (strcmp(mode, "L") == 0) {
        /* 8-bit greyscale (luminance) images */
        im->bands = im->pixelsize = 1;
        im->linesize = xsize;

    } else if (strcmp(mode, "F") == 0) {
        /* 32-bit floating point images */
        im->bands = 1;
        im->pixelsize = 4;
        im->linesize = xsize * 4;
        im->type = IMAGING_TYPE_FLOAT32;

    } else if (strcmp(mode, "I") == 0) {
        /* 32-bit integer images */
        im->bands = 1;
        im->pixelsize = 4;
        im->linesize = xsize * 4;
        im->type = IMAGING_TYPE_INT32;

    } else if (strcmp(mode, "I;16") == 0 || strcmp(mode, "I;16B") == 0) {
        /* EXPERIMENTAL */
        /* 16-bit raw integer images */
        im->bands = 1;
        im->pixelsize = 2;
        im->linesize = xsize * 2;
        im->type = IMAGING_TYPE_SPECIAL;

    } else if (strcmp(mode, "RGB") == 0) {
        /* 24-bit true colour images */
        im->bands = 3;
        im->pixelsize = 4;
        im->linesize = xsize * 4;

    } else if (strcmp(mode, "BGR;15") == 0) {
        /* EXPERIMENTAL */
        /* 15-bit true colour */
        im->bands = 1;
        im->pixelsize = 2;
        im->linesize = (xsize*2 + 3) & -4;
        im->type = IMAGING_TYPE_SPECIAL;

    } else if (strcmp(mode, "BGR;16") == 0) {
        /* EXPERIMENTAL */
        /* 16-bit reversed true colour */
        im->bands = 1;
        im->pixelsize = 2;
        im->linesize = (xsize*2 + 3) & -4;
        im->type = IMAGING_TYPE_SPECIAL;

    } else if (strcmp(mode, "BGR;24") == 0) {
        /* EXPERIMENTAL */
        /* 24-bit reversed true colour */
        im->bands = 1;
        im->pixelsize = 3;
        im->linesize = (xsize*3 + 3) & -4;
        im->type = IMAGING_TYPE_SPECIAL;

    } else if (strcmp(mode, "RGBX") == 0) {
        /* 32-bit true colour images with padding */
        im->bands = im->pixelsize = 4;
        im->linesize = xsize * 4;

    } else if (strcmp(mode, "RGBA") == 0) {
        /* 32-bit true colour images with alpha */
        im->bands = im->pixelsize = 4;
        im->linesize = xsize * 4;

    } else if (strcmp(mode, "RGBa") == 0) {
        /* EXPERIMENTAL */
        /* 32-bit true colour images with premultiplied alpha */
        im->bands = im->pixelsize = 4;
        im->linesize = xsize * 4;

    } else if (strcmp(mode, "CMYK") == 0) {
        /* 32-bit colour separation */
        im->bands = im->pixelsize = 4;
        im->linesize = xsize * 4;

    } else if (strcmp(mode, "YCbCr") == 0) {
        /* 24-bit video format */
        im->bands = 3;
        im->pixelsize = 4;
        im->linesize = xsize * 4;

    } else {
        free(im);
	return (Imaging) ImagingError_ValueError("unrecognized mode");
    }

    /* Setup image descriptor */
    strcpy(im->mode, mode);

    /* Pointer array (allocate at least one line, to avoid MemoryError
       exceptions on platforms where calloc(0, x) returns NULL) */
    im->image = (char **) calloc((ysize > 0) ? ysize : 1, sizeof(void *));
    if (!im->image) {
	free(im);
	return (Imaging) ImagingError_MemoryError();
    }

    return im;
}

Imaging
ImagingNewEpilogue(Imaging im)
{
    /* If the raster data allocator didn't setup a destructor,
       assume that it couldn't allocate the required amount of
       memory. */
    if (!im->destroy)
	return (Imaging) ImagingError_MemoryError();

    /* Initialize alias pointers to pixel data. */
    switch (im->pixelsize) {
    case 1: case 2: case 3:
	im->image8 = (UINT8 **) im->image;
	break;
    case 4:
	im->image32 = (INT32 **) im->image;
	break;
    }

    return im;
}

void
ImagingDelete(Imaging im)
{
    if (!im)
	return;

    if (im->palette)
	ImagingPaletteDelete(im->palette);

    if (im->destroy)
	im->destroy(im);

    if (im->image)
	free(im->image);

    free(im);
}


/* Array Storage Type */
/* ------------------ */
/* Allocate image as an array of line buffers. */

static void
ImagingDestroyArray(Imaging im)
{
    int y;

    if (im->image)
	for (y = 0; y < im->ysize; y++)
	    if (im->image[y])
		free(im->image[y]);
}

Imaging
ImagingNewArray(const char *mode, int xsize, int ysize)
{
    Imaging im;
    int y;
    char* p;

    im = ImagingNewPrologue(mode, xsize, ysize);
    if (!im)
	return NULL;

    /* Allocate image as an array of lines */
    for (y = 0; y < im->ysize; y++) {
	p = (char *) malloc(im->linesize);
	if (!p) {
	    ImagingDestroyArray(im);
	    break;
	}
        im->image[y] = p;
    }

    if (y == im->ysize)
	im->destroy = ImagingDestroyArray;

    return ImagingNewEpilogue(im);
}


/* Block Storage Type */
/* ------------------ */
/* Allocate image as a single block. */

static void
ImagingDestroyBlock(Imaging im)
{
    if (im->block)
	free(im->block);
}

Imaging
ImagingNewBlock(const char *mode, int xsize, int ysize)
{
    Imaging im;
    int y, i;
    int bytes;

    im = ImagingNewPrologue(mode, xsize, ysize);
    if (!im)
	return NULL;

    /* Use a single block */
    bytes = im->ysize * im->linesize;
    if (bytes <= 0)
        /* some platforms return NULL for malloc(0); this fix
           prevents MemoryError on zero-sized images on such
           platforms */
        bytes = 1;
    im->block = (char *) malloc(bytes);

    if (im->block) {

	for (y = i = 0; y < im->ysize; y++) {
	    im->image[y] = im->block + i;
	    i += im->linesize;
	}

	im->destroy = ImagingDestroyBlock;

    }

    return ImagingNewEpilogue(im);
}

/* --------------------------------------------------------------------
 * Create a new, internally allocated, image.
 */
#if defined(IMAGING_SMALL_MODEL)
#define	THRESHOLD	16384L
#else
#define	THRESHOLD	1048576L
#endif

Imaging
ImagingNew(const char* mode, int xsize, int ysize)
{
    /* FIXME: strlen(mode) is no longer accurate */
    if ((long) xsize * ysize * strlen(mode) <= THRESHOLD)
	return ImagingNewBlock(mode, xsize, ysize);
    else
	return ImagingNewArray(mode, xsize, ysize);
}

Imaging
ImagingNew2(const char* mode, Imaging imOut, Imaging imIn)
{
    /* allocate or validate output image */

    if (imOut) {
        /* make sure images match */
        if (strcmp(imOut->mode, mode) != 0
            || imOut->xsize != imIn->xsize
            || imOut->ysize != imIn->ysize) {
            ImagingError_Mismatch();
            return NULL;
        }
    } else {
        /* create new image */
        imOut = ImagingNew(mode, imIn->xsize, imIn->ysize);
        if (!imOut)
            return NULL;
    }

    return imOut;
}

void
ImagingCopyInfo(Imaging destination, Imaging source)
{
    if (source->palette)
	destination->palette = ImagingPaletteDuplicate(source->palette);
}
