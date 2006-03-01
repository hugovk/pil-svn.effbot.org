/*
 * The Python Imaging Library.
 * $Id: //modules/pil/display.c#3 $
 *
 * display support
 *
 * History:
 * 1996-05-13 fl  Windows DIB support
 * 1996-05-21 fl  Added palette stuff
 * 1996-05-28 fl  Added display_mode stuff
 * 1997-09-21 fl  Added draw primitive
 * 2001-09-17 fl  Added ImagingGrabScreen (from _grabscreen.c)
 *
 * Copyright (c) Secret Labs AB 1997.
 * Copyright (c) Fredrik Lundh 1996-97.
 *
 * See the README file for information on usage and redistribution.
 */


#include "Python.h"
#include "Imaging.h"


/* -------------------------------------------------------------------- */
/* Windows DIB support							*/
/* -------------------------------------------------------------------- */

#ifdef	WIN32

#include "ImDib.h"

typedef struct {
    PyObject_HEAD
    ImagingDIB dib;
} ImagingDisplayObject;

staticforward PyTypeObject ImagingDisplayType;

static ImagingDisplayObject*
_new(const char* mode, int xsize, int ysize)
{
    ImagingDisplayObject *display;

    display = PyObject_NEW(ImagingDisplayObject, &ImagingDisplayType);
    if (display == NULL)
	return NULL;

    display->dib = ImagingNewDIB(mode, xsize, ysize);
    if (!display->dib) {
	Py_DECREF(display);
	return NULL;
    }

    return display;
}

static void
_delete(ImagingDisplayObject* display)
{
    if (display->dib)
	ImagingDeleteDIB(display->dib);
    PyMem_DEL(display);
}

static PyObject* 
_expose(ImagingDisplayObject* display, PyObject* args)
{
    int hdc;
    if (!PyArg_ParseTuple(args, "i", &hdc))
	return NULL;

    ImagingExposeDIB(display->dib, hdc);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* 
_draw(ImagingDisplayObject* display, PyObject* args)
{
    int hdc;
    int dst[4];
    int src[4];
    if (!PyArg_ParseTuple(args, "i(iiii)(iiii)", &hdc,
                          dst+0, dst+1, dst+2, dst+3,
                          src+0, src+1, src+2, src+3))
	return NULL;

    ImagingDrawDIB(display->dib, hdc, dst, src);

    Py_INCREF(Py_None);
    return Py_None;
}

extern Imaging PyImaging_AsImaging(PyObject *op);

static PyObject*
_paste(ImagingDisplayObject* display, PyObject* args)
{
    Imaging im;

    PyObject* op;
    int xy[4];
    xy[0] = xy[1] = xy[2] = xy[3] = 0;
    if (!PyArg_ParseTuple(args, "O|(iiii)", &op, xy+0, xy+1, xy+2, xy+3))
	return NULL;
    im = PyImaging_AsImaging(op);
    if (!im)
	return NULL;

    if (xy[2] <= xy[0])
	xy[2] = xy[0] + im->xsize;
    if (xy[3] <= xy[1])
	xy[3] = xy[1] + im->ysize;

    ImagingPasteDIB(display->dib, im, xy);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* 
_query_palette(ImagingDisplayObject* display, PyObject* args)
{
    int hdc;
    int status;

    if (!PyArg_ParseTuple(args, "i", &hdc))
	return NULL;

    status = ImagingQueryPaletteDIB(display->dib, hdc);

    return Py_BuildValue("i", status);
}

static struct PyMethodDef methods[] = {
    {"draw", (PyCFunction)_draw, 1},
    {"expose", (PyCFunction)_expose, 1},
    {"paste", (PyCFunction)_paste, 1},
    {"query_palette", (PyCFunction)_query_palette, 1},
    {NULL, NULL} /* sentinel */
};

static PyObject*  
_getattr(ImagingDisplayObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(methods, (PyObject*) self, name);
    if (res)
	return res;
    PyErr_Clear();
    if (!strcmp(name, "mode"))
	return Py_BuildValue("s", self->dib->mode);
    if (!strcmp(name, "size"))
	return Py_BuildValue("ii", self->dib->xsize, self->dib->ysize);
    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

statichere PyTypeObject ImagingDisplayType = {
	PyObject_HEAD_INIT(NULL)
	0,				/*ob_size*/
	"ImagingDisplay",		/*tp_name*/
	sizeof(ImagingDisplayObject),	/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)_delete,		/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)_getattr,		/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
	0,                              /*tp_hash*/
};

PyObject*
PyImaging_DisplayWin32(PyObject* self, PyObject* args)
{
    ImagingDisplayObject* display;
    char *mode;
    int xsize, ysize;

    if (!PyArg_ParseTuple(args, "s(ii)", &mode, &xsize, &ysize))
	return NULL;

    display = _new(mode, xsize, ysize);
    if (display == NULL)
	return NULL;

    return (PyObject*) display;
}

PyObject*
PyImaging_DisplayModeWin32(PyObject* self, PyObject* args)
{
    char *mode;
    int size[2];

    mode = ImagingGetModeDIB(size);

    return Py_BuildValue("s(ii)", mode, size[0], size[1]);
}

PyObject*
PyImaging_GrabScreenWin32(PyObject* self, PyObject* args)
{
    int width, height;
    HBITMAP bitmap;
    BITMAPCOREHEADER core;
    HDC screen, screen_copy;
    PyObject* buffer;
    
    /* step 1: create a memory DC large enough to hold the
       entire screen */

    screen = CreateDC("DISPLAY", NULL, NULL, NULL); 
    screen_copy = CreateCompatibleDC(screen); 

    width = GetDeviceCaps(screen, HORZRES);
    height = GetDeviceCaps(screen, VERTRES);
 
    bitmap = CreateCompatibleBitmap(screen, width, height);
    if (!bitmap)
        goto error;
        
    if (!SelectObject(screen_copy, bitmap))
        goto error;

    /* step 2: copy bits into memory DC bitmap */

    if (!BitBlt(screen_copy, 0, 0, width, height, screen, 0, 0, SRCCOPY))
        goto error;

    /* step 3: extract bits from bitmap */

    buffer = PyString_FromStringAndSize(NULL, height * ((width*3 + 3) & -4));
    if (!buffer)
        return NULL;

    core.bcSize = sizeof(core);
    core.bcWidth = width;
    core.bcHeight = height;
    core.bcPlanes = 1;
    core.bcBitCount = 24;
    if (!GetDIBits(screen_copy, bitmap, 0, height, PyString_AS_STRING(buffer),
                   (BITMAPINFO*) &core, DIB_RGB_COLORS))
        goto error;

    DeleteObject(bitmap);
    DeleteDC(screen_copy);
    DeleteDC(screen);

    return Py_BuildValue("(ii)N", width, height, buffer);

error:
    PyErr_SetString(PyExc_IOError, "screen grab failed");

    DeleteDC(screen_copy);
    DeleteDC(screen);

    return NULL;
}

#endif /* WIN32 */
