/*
 * The Python Imaging Library.
 * $Id$
 *
 * a simple drawing package for the Imaging library
 *
 * history:
 * 96-04-13 fl	Created.
 * 96-04-30 fl	Added transforms and polygon support.
 * 96-08-12 fl	Added filled polygons.
 * 96-11-05 fl	Fixed float/int confusion in polygon filler
 * 97-07-04 fl	Support 32-bit images (C++ would have been nice)
 * 98-09-09 fl	Eliminated qsort casts; improved rectangle clipping
 * 98-09-10 fl	Fixed fill rectangle to include lower edge (!)
 * 98-12-29 fl	Added arc, chord, and pieslice primitives
 * 99-01-10 fl	Added some level 2 ("arrow") stuff (experimental)
 * 99-02-06 fl	Added bitmap primitive
 * 99-07-26 fl	Eliminated a compiler warning
 * 99-07-31 fl	Pass ink as void* instead of int
 *
 * Copyright (c) Fredrik Lundh 1996-97.
 * Copyright (c) Secret Labs AB 1997-99.
 *
 * See the README file for information on usage and redistribution.
 */

/* FIXME: support fill/outline attribute for all filled shapes */
/* FIXME: support zero-winding fill */
/* FIXME: support affine transform */
/* FIXME: support clip window (and mask?) */

#include <math.h>
#include "Imaging.h"

#define CEIL(v)  (int) ceil(v)
#define FLOOR(v) ((v) >= 0.0 ? (int) (v) : (int) floor(v))

#define INK8(ink) (*(UINT8*)ink)
#define INK32(ink) (*(INT32*)ink)

/* -------------------------------------------------------------------- */
/* Primitives								*/
/* -------------------------------------------------------------------- */

typedef struct {
    /* edge descriptor for polygon engine */
    int d;
    int x0, y0;
    int xmin, ymin, xmax, ymax;
    float dx;
} Edge;

static inline void
point8(Imaging im, int x, int y, int ink)
{
    if (x >= 0 && x < im->xsize && y >= 0 && y < im->ysize)
	im->image8[y][x] = (UINT8) ink;
}

static inline void
point32(Imaging im, int x, int y, int ink)
{
    if (x >= 0 && x < im->xsize && y >= 0 && y < im->ysize)
	im->image32[y][x] = ink;
}

static inline void
hline8(Imaging im, int x0, int y0, int x1, int ink)
{
    int tmp;

    if (y0 >= 0 && y0 < im->ysize) {
	if (x0 > x1)
	    tmp = x0, x0 = x1, x1 = tmp;
	if (x0 < 0)
	    x0 = 0;
	else if (x0 >= im->xsize)
	    return;
	if (x1 < 0)
	    return;
	else if (x1 >= im->xsize)
	    x1 = im->xsize-1;
	if (x0 <= x1)
	    memset(im->image8[y0] + x0, (UINT8) ink, x1 - x0 + 1);
    }
}

static inline void
hline32(Imaging im, int x0, int y0, int x1, int ink)
{
    int tmp;
    INT32* p;

    if (y0 >= 0 && y0 < im->ysize) {
	if (x0 > x1)
	    tmp = x0, x0 = x1, x1 = tmp;
	if (x0 < 0)
	    x0 = 0;
	else if (x0 >= im->xsize)
	    return;
	if (x1 < 0)
	    return;
	else if (x1 >= im->xsize)
	    x1 = im->xsize-1;
        p = im->image32[y0];
	while (x0 <= x1)
            p[x0++] = ink;
    }
}

static inline void
line8(Imaging im, int x0, int y0, int x1, int y1, int ink)
{
    int i, n, e;
    int dx, dy;
    int xs, ys;

    /* normalize coordinates */
    dx = x1-x0;
    if (dx < 0)
	dx = -dx, xs = -1;
    else
	xs = 1;
    dy = y1-y0;
    if (dy < 0)
	dy = -dy, ys = -1;
    else
	ys = 1;

    n = (dx > dy) ? dx : dy;

    if (dx == 0)

	/* vertical */
	for (i = 0; i < dy; i++) {
	    point8(im, x0, y0, ink);
	    y0 += ys;
	}

    else if (dy == 0)

	/* horizontal */
	hline8(im, x0, y0, x1, ink);

    else if (dx > dy) {

	/* bresenham, horizontal slope */
	n = dx;
	dy += dy;
	e = dy - dx;
	dx += dx;

	for (i = 0; i < n; i++) {
	    point8(im, x0, y0, ink);
	    if (e >= 0) {
		y0 += ys;
		e -= dx;
	    }
	    e += dy;
	    x0 += xs;
	}

    } else {

	/* bresenham, vertical slope */
	n = dy;
	dx += dx;
	e = dx - dy;
	dy += dy;

	for (i = 0; i < n; i++) {
	    point8(im, x0, y0, ink);
	    if (e >= 0) {
		x0 += xs;
		e -= dy;
	    }
	    e += dx;
	    y0 += ys;
	}

    }
}

static inline void
line32(Imaging im, int x0, int y0, int x1, int y1, int ink)
{
    int i, n, e;
    int dx, dy;
    int xs, ys;

    /* normalize coordinates */
    dx = x1-x0;
    if (dx < 0)
	dx = -dx, xs = -1;
    else
	xs = 1;
    dy = y1-y0;
    if (dy < 0)
	dy = -dy, ys = -1;
    else
	ys = 1;

    n = (dx > dy) ? dx : dy;

    if (dx == 0)

	/* vertical */
	for (i = 0; i < dy; i++) {
	    point32(im, x0, y0, ink);
	    y0 += ys;
	}

    else if (dy == 0)

	/* horizontal */
	hline32(im, x0, y0, x1, ink);

    else if (dx > dy) {

	/* bresenham, horizontal slope */
	n = dx;
	dy += dy;
	e = dy - dx;
	dx += dx;

	for (i = 0; i < n; i++) {
	    point32(im, x0, y0, ink);
	    if (e >= 0) {
		y0 += ys;
		e -= dx;
	    }
	    e += dy;
	    x0 += xs;
	}

    } else {

	/* bresenham, vertical slope */
	n = dy;
	dx += dx;
	e = dx - dy;
	dy += dy;

	for (i = 0; i < n; i++) {
	    point32(im, x0, y0, ink);
	    if (e >= 0) {
		x0 += xs;
		e -= dy;
	    }
	    e += dx;
	    y0 += ys;
	}

    }
}

static int
x_cmp(const void *x0, const void *x1)
{
    return *((float*)x0) - *((float*)x1);
}

static inline int
polygon8(Imaging im, int n, Edge *e, int ink, int eofill)
{
    int i, j;
    float *xx;
    int ymin, ymax;
    float y;

    if (n <= 0)
	return 0;

    /* Find upper and lower polygon boundary (within image) */

    ymin = e[0].ymin;
    ymax = e[0].ymax;
    for (i = 1; i < n; i++) {
	if (e[i].ymin < ymin) ymin = e[i].ymin;
	if (e[i].ymax > ymax) ymax = e[i].ymax;
    }

    if (ymin < 0)
	ymin = 0;
    if (ymax >= im->ysize)
	ymax = im->ysize-1;

    /* Process polygon edges */

    xx = malloc(n * sizeof(float));
    if (!xx)
	return -1;

    for (;ymin <= ymax; ymin++) {
	y = ymin+0.5;
	for (i = j = 0; i < n; i++) 
	    if (y >= e[i].ymin && y <= e[i].ymax)
		if (e[i].d == 0)
		    hline8(im, e[i].xmin, ymin, e[i].xmax, ink);
		else
		    xx[j++] = (y-e[i].y0) * e[i].dx + e[i].x0;
	if (j == 2) {
            if (xx[0] < xx[1])
                hline8(im, CEIL(xx[0]-0.5), ymin, FLOOR(xx[1]+0.5), ink);
            else
                hline8(im, CEIL(xx[1]-0.5), ymin, FLOOR(xx[0]+0.5), ink);
	} else {
	    qsort(xx, j, sizeof(float), x_cmp);
	    for (i = 0; i < j-1 ; i += 2)
		hline8(im, CEIL(xx[i]-0.5), ymin, FLOOR(xx[i+1]+0.5), ink);
	}
    }

    free(xx);

    return 0;
}

static inline int
polygon32(Imaging im, int n, Edge *e, int ink, int eofill)
{
    int i, j;
    float *xx;
    int ymin, ymax;
    float y;

    if (n <= 0)
	return 0;

    /* Find upper and lower polygon boundary (within image) */

    ymin = e[0].ymin;
    ymax = e[0].ymax;
    for (i = 1; i < n; i++) {
	if (e[i].ymin < ymin) ymin = e[i].ymin;
	if (e[i].ymax > ymax) ymax = e[i].ymax;
    }

    if (ymin < 0)
	ymin = 0;
    if (ymax >= im->ysize)
	ymax = im->ysize-1;

    /* Process polygon edges */

    xx = malloc(n * sizeof(float));
    if (!xx)
	return -1;

    for (;ymin <= ymax; ymin++) {
	y = ymin+0.5;
	for (i = j = 0; i < n; i++) {
	    if (y >= e[i].ymin && y <= e[i].ymax)
		if (e[i].d == 0)
		    hline32(im, e[i].xmin, ymin, e[i].xmax, ink);
		else
		    xx[j++] = (y-e[i].y0) * e[i].dx + e[i].x0;
        }
	if (j == 2) {
            if (xx[0] < xx[1])
                hline32(im, CEIL(xx[0]-0.5), ymin, FLOOR(xx[1]+0.5), ink);
            else
                hline32(im, CEIL(xx[1]-0.5), ymin, FLOOR(xx[0]+0.5), ink);
	} else {
	    qsort(xx, j, sizeof(float), x_cmp);
	    for (i = 0; i < j-1 ; i += 2)
                hline32(im, CEIL(xx[i]-0.5), ymin, FLOOR(xx[i+1]+0.5), ink);
	}
    }

    free(xx);

    return 0;
}

static inline void
add_edge(Edge *e, int x0, int y0, int x1, int y1)
{
    /* printf("edge %d %d %d %d\n", x0, y0, x1, y1); */

    if (x0 <= x1)
	e->xmin = x0, e->xmax = x1;
    else
	e->xmin = x1, e->xmax = x0;

    if (y0 <= y1)
	e->ymin = y0, e->ymax = y1;
    else
	e->ymin = y1, e->ymax = y0;
    
    if (y0 == y1) {
	e->d = 0;
	e->dx = 0.0;
    } else {
	e->dx = ((float)(x1-x0)) / (y1-y0);
	if (y0 == e->ymin)
	    e->d = 1;
	else
	    e->d = -1;
    }

    e->x0 = x0;
    e->y0 = y0;
}

typedef struct {
    void (*point)(Imaging im, int x, int y, int ink);
    void (*hline)(Imaging im, int x0, int y0, int x1, int ink);
    void (*line)(Imaging im, int x0, int y0, int x1, int y1, int ink);
    int (*polygon)(Imaging im, int n, Edge *e, int ink, int eofill);
} DRAW;

DRAW draw8  = { point8,  hline8,  line8,  polygon8 };
DRAW draw32 = { point32, hline32, line32, polygon32 };


/* -------------------------------------------------------------------- */
/* Interface								*/
/* -------------------------------------------------------------------- */

int
ImagingDrawPoint(Imaging im, int x0, int y0, const void* ink)
{
    if (im->image8)
        draw8.point(im, x0, y0, INK8(ink));
    else
        draw32.point(im, x0, y0, INK32(ink));

    return 0;
}

int
ImagingDrawLine(Imaging im, int x0, int y0, int x1, int y1, const void* ink)
{
    if (im->image8)
        draw8.line(im, x0, y0, x1, y1, INK8(ink));
    else
        draw32.line(im, x0, y0, x1, y1, INK32(ink));

    return 0;
}

int
ImagingDrawRectangle(Imaging im, int x0, int y0, int x1, int y1,
		     const void* ink_, int fill)
{
    int y;
    int tmp;
    DRAW* draw;
    INT32 ink;

    if (im->image8) {
        draw = &draw8;
        ink = INK8(ink_);
    } else {
        draw = &draw32;
        ink = INK32(ink_);
    }
 
    if (y0 > y1)
	tmp = y0, y0 = y1, y1 = tmp;

    if (fill) {

        if (y0 < 0)
            y0 = 0;
        else if (y0 >= im->ysize)
            return 0;

        if (y1 < 0)
            return 0;
        else if (y1 > im->ysize)
            y1 = im->ysize;

	for (y = y0; y <= y1; y++)
	    draw->hline(im, x0, y, x1, ink);

    } else {

	/* outline */
	draw->line(im, x0, y0, x1, y0, ink);
	draw->line(im, x1, y0, x1, y1, ink);
	draw->line(im, x1, y1, x0, y1, ink);
	draw->line(im, x0, y1, x0, y0, ink);

    }

    return 0;
}

int
ImagingDrawPolygon(Imaging im, int count, int* xy, const void* ink_, int fill)
{
    int i, n;
    DRAW* draw;
    INT32 ink;

    if (count <= 0)
	return 0;

    if (im->image8) {
        draw = &draw8;
        ink = INK8(ink_);
    } else {
        draw = &draw32;
        ink = INK32(ink_);
    }

    if (fill) {

	/* Build edge list */
	Edge* e = malloc(count * sizeof(Edge));
	if (!e) {
	    ImagingError_MemoryError();
	    return -1;
	}
	for (i = n = 0; i < count-1; i++)
	    add_edge(&e[n++], xy[i+i], xy[i+i+1], xy[i+i+2], xy[i+i+3]);
	if (xy[i+i] != xy[0] || xy[i+i+1] != xy[1])
	    add_edge(&e[n++], xy[i+i], xy[i+i+1], xy[0], xy[1]);
	draw->polygon(im, n, e, ink, 0);
	free(e);

    } else {

	/* Outline */
	for (i = 0; i < count-1; i++)
	    draw->line(im, xy[i+i], xy[i+i+1], xy[i+i+2], xy[i+i+3], ink);
	draw->line(im, xy[i+i], xy[i+i+1], xy[0], xy[1], ink);

    }
	
    return 0;
}

int
ImagingDrawBitmap(Imaging im, int x0, int y0, Imaging bitmap, const void* ink)
{
    return ImagingFill2(
        im, ink, bitmap,
        x0, y0, x0 + bitmap->xsize, y0 + bitmap->ysize
        );
}

/* -------------------------------------------------------------------- */
/* standard shapes */

#define ARC 0
#define CHORD 1
#define PIESLICE 2

static int
ellipse(Imaging im, int x0, int y0, int x1, int y1,
        int start, int end, const void* ink_, int fill,
        int mode)
{
    int i, n;
    int cx, cy;
    int w, h;
    int x, y;
    int lx, ly;
    int sx, sy;
    DRAW* draw;
    INT32 ink;

    w = x1 - x0;
    h = y1 - y0;
    if (w < 0 || h < 0)
        return 0;

    if (im->image8) {
        draw = &draw8;
        ink = INK8(ink_);
    } else {
        draw = &draw32;
        ink = INK32(ink_);
    }

    cx = (x0 + x1) / 2;
    cy = (y0 + y1) / 2;

    while (end < start)
	end += 360;

    if (mode != ARC && fill) {

        /* Build edge list */
	Edge* e = malloc((end - start + 3) * sizeof(Edge));
        if (!e) {
            ImagingError_MemoryError();
            return -1;
        }

        n = 0;

        for (i = start; i <= end; i++) {
            x = FLOOR((cos(i*M_PI/180) * w/2) + cx + 0.5);
            y = FLOOR((sin(i*M_PI/180) * h/2) + cy + 0.5);
            if (i != start)
                add_edge(&e[n++], lx, ly, x, y);
            else
                sx = x, sy = y;
            lx = x, ly = y;
        }

        if (n > 0) {
            /* close and draw polygon */
            if (mode == PIESLICE) {
                if (x != cx || y != cy) {
                    add_edge(&e[n++], x, y, cx, cy);
                    add_edge(&e[n++], cx, cy, sx, sy);
                }
            } else {
                if (x != sx || y != sy)
                    add_edge(&e[n++], x, y, sx, sy);
            }
            draw->polygon(im, n, e, ink, 0);
        }

        free(e);
        
    } else {

        for (i = start; i <= end; i++) {
            x = FLOOR((cos(i*M_PI/180) * w/2) + cx + 0.5);
            y = FLOOR((sin(i*M_PI/180) * h/2) + cy + 0.5);
            if (i != start)
                draw->line(im, lx, ly, x, y, ink);
            else
                sx = x, sy = y;
            lx = x, ly = y;
        }

        if (i != start) {
            if (mode == PIESLICE) {
                if (x != cx || y != cy) {
                    draw->line(im, x, y, cx, cy, ink);
                    draw->line(im, cx, cy, sx, sy, ink);
                }
            } else if (mode == CHORD) {
                if (x != sx || y != sy)
                    draw->line(im, x, y, sx, sy, ink);
            }
        }
    }

    return 0;
}

int
ImagingDrawArc(Imaging im, int x0, int y0, int x1, int y1,
               int start, int end, const void* ink)
{
    return ellipse(im, x0, y0, x1, y1, start, end, ink, 0, ARC);
}

int
ImagingDrawChord(Imaging im, int x0, int y0, int x1, int y1,
               int start, int end, const void* ink, int fill)
{
    return ellipse(im, x0, y0, x1, y1, start, end, ink, fill, CHORD);
}

int
ImagingDrawEllipse(Imaging im, int x0, int y0, int x1, int y1,
                   const void* ink, int fill)
{
    return ellipse(im, x0, y0, x1, y1, 0, 360, ink, fill, CHORD);
}

int
ImagingDrawPieslice(Imaging im, int x0, int y0, int x1, int y1,
                    int start, int end, const void* ink, int fill)
{
    return ellipse(im, x0, y0, x1, y1, start, end, ink, fill, PIESLICE);
}

/* -------------------------------------------------------------------- */

/* experimental level 2 ("arrow") graphics stuff.  this implements
   portions of the arrow api on top of the Edge structure.  the
   semantics are ok, except that "curve" flattens the bezier curves by
   itself */

#if 1 /* ARROW_GRAPHICS */

struct ImagingOutlineInstance {

    float x0, y0;

    float x, y;

    int count;
    Edge *edges;

    int size;

};


ImagingOutline
ImagingOutlineNew(void)
{
    ImagingOutline outline;

    outline = calloc(1, sizeof(struct ImagingOutlineInstance));
    if (!outline)
	return (ImagingOutline) ImagingError_MemoryError();

    outline->edges = NULL;
    outline->count = outline->size = 0;

    ImagingOutlineMove(outline, 0, 0);

    return outline;
}

void
ImagingOutlineDelete(ImagingOutline outline)
{
    if (!outline)
	return;

    if (outline->edges)
        free(outline->edges);

    free(outline);
}


static Edge*
allocate(ImagingOutline outline, int extra)
{
    Edge* e;

    if (outline->count + extra > outline->size) {
        /* expand outline buffer */
        outline->size += extra + 25;
        if (!outline->edges)
            e = malloc(outline->size * sizeof(Edge));
        else
            e = realloc(outline->edges, outline->size * sizeof(Edge));
        if (!e)
            return NULL;
        outline->edges = e;
    }

    e = outline->edges + outline->count;

    outline->count += extra;

    return e;
}

int
ImagingOutlineMove(ImagingOutline outline, float x0, float y0)
{
    outline->x = outline->x0 = x0;
    outline->y = outline->y0 = y0;

    return 0;
}

int
ImagingOutlineLine(ImagingOutline outline, float x1, float y1)
{
    Edge* e;

    e = allocate(outline, 1);
    if (!e)
        return -1; /* out of memory */

    add_edge(e, (int) outline->x, (int) outline->y, (int) x1, (int) y1);

    outline->x = x1;
    outline->y = y1;
    
    return 0;
}

int
ImagingOutlineCurve(ImagingOutline outline, float x1, float y1,
                    float x2, float y2, float x3, float y3)
{
    Edge* e;
    int i;
    float xo, yo;

#define STEPS 32

    e = allocate(outline, STEPS);
    if (!e)
        return -1; /* out of memory */

    xo = outline->x;
    yo = outline->y;

    /* flatten the bezier segment */

    for (i = 1; i <= STEPS; i++) {

        float t = ((float) i) / STEPS;
        float t2 = t*t;
        float t3 = t2*t;

        float u = 1.0 - t;
        float u2 = u*u;
        float u3 = u2*u;

        float x = outline->x*u3 + 3*(x1*t*u2 + x2*t2*u) + x3*t3;
        float y = outline->y*u3 + 3*(y1*t*u2 + y2*t2*u) + y3*t3;

        add_edge(e++, xo, yo, x, y);

        xo = x, yo = y;

    }

    outline->x = xo;
    outline->y = yo;
    
    return 0;
}

int
ImagingOutlineCurve2(ImagingOutline outline, float cx, float cy,
                     float x3, float y3) 
{
    /* add bezier curve based on three control points (as
       in the Flash file format) */

    /* FIXME: Flash uses quadratic beziers, not cubics.  one day, I
       should really figure if this approximation is correct or not.
       looks pretty ok, though... */

    return ImagingOutlineCurve(
        outline,
        (outline->x + cx + cx)/3, (outline->y + cy + cy)/3,
        (cx + cx + x3)/3, (cy + cy + y3)/3,
        x3, y3);
}

int
ImagingOutlineClose(ImagingOutline outline)
{
    if (outline->x == outline->x0 && outline->y == outline->y0)
        return 0;
    return ImagingOutlineLine(outline, outline->x0, outline->y0);
}

int
ImagingOutlineTransform(ImagingOutline outline, double a[6])
{
    Edge *eIn;
    Edge *eOut;
    int i, n;
    int x0, y0, x1, y1;
    int X0, Y0, X1, Y1;
    
    double a0 = a[0]; double a1 = a[1]; double a2 = a[2];
    double a3 = a[3]; double a4 = a[4]; double a5 = a[5];

    eIn = outline->edges;
    n = outline->count;
    
    /* FIXME: ugly! */
    outline->edges = NULL;
    outline->count = outline->size = 0;

    eOut = allocate(outline, n);
    if (!eOut) {
        outline->edges = eIn;
        outline->count = outline->size = n;
        ImagingError_MemoryError();
        return -1;
    }
    
    for (i = 0; i < n; i++) {
        
        x0 = eIn->x0;
        y0 = eIn->y0;
        
        /* FIXME: ouch! */
        if (eIn->x0 == eIn->xmin)
            x1 = eIn->xmax;
        else
            x1 = eIn->xmin;
        if (eIn->y0 == eIn->ymin)
            y1 = eIn->ymax;
        else
            y1 = eIn->ymin;

        /* full moon tonight!  strange spells needed to prevent
           compiler errors.  (or you could install the appropriate
           service pack...) */

        X0 = (int) (a0*x0 + a1*y0 + a2);

        /* printf(""); */

        Y0 = (int) (a3*x0 + a4*y0 + a5);

        /* printf(""); */

        X1 = (int) (a0*x1 + a1*y1 + a2);

        /* printf(""); */

        Y1 = (int) (a3*x1 + a4*y1 + a5);

        /* printf(""); */

        add_edge(eOut, X0, Y0, X1, Y1);

        eIn++;
        eOut++;

    }

    free(eIn);

    return 0;
}

int
ImagingDrawOutline(Imaging im, ImagingOutline outline, const void* ink,
                   int fill)
{
    if (im->image8)
        draw8.polygon(im, outline->count, outline->edges, INK8(ink), 0);
    else
        draw32.polygon(im, outline->count, outline->edges, INK32(ink), 0);

    return 0;
}

#endif
