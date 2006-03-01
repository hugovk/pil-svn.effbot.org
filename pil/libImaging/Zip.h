/*
 * The Python Imaging Library.
 * $Id$
 *
 * declarations for the ZIP codecs
 *
 * Copyright (c) Fredrik Lundh 1996.  All rights reserved.
 */


#include "zlib.h"


/* modes */
#define	ZIP_PNG	0		/* continuous, filtered image data */
#define	ZIP_PNG_PALETTE	1	/* non-continuous data, disable filtering */
#define	ZIP_TIFF_PREDICTOR 2	/* TIFF, with predictor */
#define	ZIP_TIFF 3		/* TIFF, without predictor */


typedef struct {

    /* CONFIGURATION */

    /* Codec mode */
    int mode;

    /* Optimize (max compression) SLOW!!! */
    int optimize;

    /* Predefined dictionary (experimental) */
    char* dictionary;
    int dictionary_size;

    /* PRIVATE CONTEXT (set by decoder/encoder) */

    z_stream z_stream;		/* (de)compression stream */

    UINT8* previous;		/* previous line (allocated) */

    /* Compressor specific stuff */
    UINT8* prior;		/* filter storage (allocated) */
    UINT8* up;
    UINT8* average;
    UINT8* paeth;

    UINT8* output;		/* output data */

    int prefix;			/* size of filter prefix (0 for TIFF data) */

} ZIPSTATE;
