/* 
 * The Python Imaging Library
 * $Id$
 *
 * platform declarations for the imaging core library
 *
 * Copyright (c) Fredrik Lundh 1995-96.  All rights reserved.
 */


#include "ImConfig.h"


/* For now, we require an ANSI compliant compiler */
#if HAVE_PROTOTYPES == 0
#error Sorry, this library uses ANSI prototypes.
#endif
#if STDC_HEADERS == 0
#error Sorry, this library assumes ANSI header files.
#endif

#if SIZEOF_SHORT == 2
#define	INT16 short
#elif SIZEOF_INT == 2
#define	INT16 int
#else
#define	INT16 short /* most things works just fine anyway... */
#endif

#if SIZEOF_SHORT == 4
#define	INT32 short
#elif SIZEOF_INT == 4
#define	INT32 int
#elif SIZEOF_LONG == 4
#define	INT32 long
#else
#error Cannot find required 32-bit integer type
#endif

#if SIZEOF_LONG == 8
#define	INT64 long
#endif

#if SIZEOF_FLOAT == 4
#define	FLOAT32 float
#elif SIZEOF_DOUBLE == 4
#define	FLOAT32 double /* and pigs can fly... */
#else
#error Cannot find required 32-bit floating point type
#endif

#if SIZEOF_DOUBLE == 8
#define	FLOAT64 double
#endif

#define	INT8  signed char
#define	UINT8 unsigned char

#define	UINT16 unsigned INT16
#define	UINT32 unsigned INT32
