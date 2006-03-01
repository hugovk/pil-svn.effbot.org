/*
 * The Python Imaging Library.
 * $Id: //modules/pil/display.c#7 $
 *
 * display support
 *
 * History:
 * 1996-05-13 fl  Windows DIB support
 * 1996-05-21 fl  Added palette stuff
 * 1996-05-28 fl  Added display_mode stuff
 * 1997-09-21 fl  Added draw primitive
 * 2001-09-17 fl  Added ImagingGrabScreen (from _grabscreen.c)
 * 2002-05-12 fl  Added ImagingListWindows
 * 2002-11-19 fl  Added clipboard support
 * 2002-11-25 fl  Added GetDC/ReleaseDC helpers
 *
 * Copyright (c) 1997-2002 by Secret Labs AB.
 * Copyright (c) 1996-1997 by Fredrik Lundh.
 *
 * See the README file for information on usage and redistribution.
 */


#include "Python.h"

#if PY_VERSION_HEX < 0x01060000
#define PyObject_DEL(op) PyMem_DEL((op))
#endif

#include "Imaging.h"

/* -------------------------------------------------------------------- */
/* Windows DIB support	*/

#ifdef WIN32

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
    PyObject_DEL(display);
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

static PyObject*
_getdc(ImagingDisplayObject* display, PyObject* args)
{
    int window;
    HDC dc;

    if (!PyArg_ParseTuple(args, "i", &window))
	return NULL;

    dc = GetDC((HWND) window);
    if (!dc) {
        PyErr_SetString(PyExc_IOError, "cannot create dc");
        return NULL;
    }

    return Py_BuildValue("i", (int) dc);
}

static PyObject*
_releasedc(ImagingDisplayObject* display, PyObject* args)
{
    int window, dc;

    if (!PyArg_ParseTuple(args, "ii", &window, &dc))
	return NULL;

    ReleaseDC((HWND) window, (HDC) dc);

    Py_INCREF(Py_None);
    return Py_None;
}

static struct PyMethodDef methods[] = {
    {"draw", (PyCFunction)_draw, 1},
    {"expose", (PyCFunction)_expose, 1},
    {"paste", (PyCFunction)_paste, 1},
    {"query_palette", (PyCFunction)_query_palette, 1},
    {"getdc", (PyCFunction)_getdc, 1},
    {"releasedc", (PyCFunction)_releasedc, 1},
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

/* -------------------------------------------------------------------- */
/* Windows screen grabber */

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

static BOOL CALLBACK list_windows_callback(HWND hwnd, LPARAM lParam)
{
    PyObject* window_list = (PyObject*) lParam;
    PyObject* item;
    PyObject* title;
    RECT inner, outer;
    int title_size;
    int status;
    
    /* get window title */
    title_size = GetWindowTextLength(hwnd);
    if (title_size > 0) {
        title = PyString_FromStringAndSize(NULL, title_size);
        if (title)
            GetWindowText(hwnd, PyString_AS_STRING(title), title_size+1);
    } else
        title = PyString_FromString("");
    if (!title)
        return 0;

    /* get bounding boxes */
    GetClientRect(hwnd, &inner);
    GetWindowRect(hwnd, &outer);

    item = Py_BuildValue(
        "lN(iiii)(iiii)", (long) hwnd, title,
        inner.left, inner.top, inner.right, inner.bottom,
        outer.left, outer.top, outer.right, outer.bottom
        );
    if (!item)
        return 0;

    status = PyList_Append(window_list, item);

    Py_DECREF(item);

    if (status < 0)
        return 0;
    
    return 1;
}

PyObject*
PyImaging_ListWindowsWin32(PyObject* self, PyObject* args)
{
    PyObject* window_list;
    
    window_list = PyList_New(0);
    if (!window_list)
        return NULL;

    EnumWindows(list_windows_callback, (LPARAM) window_list);

    if (PyErr_Occurred()) {
        Py_DECREF(window_list);
        return NULL;
    }

    return window_list;
}

PyObject*
PyImaging_GrabClipboardWin32(PyObject* self, PyObject* args)
{
    int clip;
    HANDLE handle;
    int size;
    void* data;
    PyObject* result;
    
    int verbose = 0; /* debugging; will be removed in future versions */
    if (!PyArg_ParseTuple(args, "|i", &verbose))
	return NULL;


    clip = OpenClipboard(NULL);
    /* FIXME: check error status */
    
    if (verbose) {
        UINT format = EnumClipboardFormats(0);
        char buffer[200];
        char* result;
        while (format != 0) {
            if (GetClipboardFormatName(format, buffer, sizeof buffer) > 0)
                result = buffer;
            else
                switch (format) {
                case CF_BITMAP:
                    result = "CF_BITMAP";
                    break;
                case CF_DIB:
                    result = "CF_DIB";
                    break;
                case CF_DIF:
                    result = "CF_DIF";
                    break;
                case CF_ENHMETAFILE:
                    result = "CF_ENHMETAFILE";
                    break;
                case CF_HDROP:
                    result = "CF_HDROP";
                    break;
                case CF_LOCALE:
                    result = "CF_LOCALE";
                    break;
                case CF_METAFILEPICT:
                    result = "CF_METAFILEPICT";
                    break;
                case CF_OEMTEXT:
                    result = "CF_OEMTEXT";
                    break;
                case CF_OWNERDISPLAY:
                    result = "CF_OWNERDISPLAY";
                    break;
                case CF_PALETTE:
                    result = "CF_PALETTE";
                    break;
                case CF_PENDATA:
                    result = "CF_PENDATA";
                    break;
                case CF_RIFF:
                    result = "CF_RIFF";
                    break;
                case CF_SYLK:
                    result = "CF_SYLK";
                    break;
                case CF_TEXT:
                    result = "CF_TEXT";
                    break;
                case CF_WAVE:
                    result = "CF_WAVE";
                    break;
                case CF_TIFF:
                    result = "CF_TIFF";
                    break;
                case CF_UNICODETEXT:
                    result = "CF_UNICODETEXT";
                    break;
                default:
                    sprintf(buffer, "[%d]", format);
                    result = buffer;
                    break;
                }
            printf("%s (%d)\n", result, format);
            format = EnumClipboardFormats(format);
        }
    }

    handle = GetClipboardData(CF_DIB);
    if (!handle) {
        /* FIXME: add CF_HDROP support to allow cut-and-paste from
           the explorer */
        Py_INCREF(Py_None);
        return Py_None;
    }

    size = GlobalSize(handle);
    data = GlobalLock(handle);

#if 0
    /* calculate proper size for string formats */
    if (format == CF_TEXT || format == CF_OEMTEXT)
        size = strlen(data);
    else if (format == CF_UNICODETEXT)
        size = wcslen(data) * 2;
#endif

    result = PyString_FromStringAndSize(data, size);

    GlobalUnlock(handle);

    CloseClipboard();

    return result;
}

#endif /* WIN32 */
