/*
 * PIL FreeType Driver
 * $Id: //modules/pil/_imagingft.c#7 $
 *
 * a FreeType 2.0 driver for PIL
 *
 * history:
 * 2001-02-17 fl  Created (based on old experimental freetype 1.0 code)
 * 2001-04-18 fl  Fixed some egcs compiler nits
 * 2002-11-08 fl  Added unicode support; more font metrics, etc
 *
 * Copyright (c) 1998-2001 by Secret Labs AB
 */

#include "Python.h"
#include "Imaging.h"

#include <freetype/freetype.h>

#if PY_VERSION_HEX < 0x01060000
#define PyObject_DEL(op) PyMem_DEL((op))
#endif

/* -------------------------------------------------------------------- */
/* font objects */

static FT_Library library;

typedef struct {
    PyObject_HEAD
    FT_Face face;
} FontObject;

staticforward PyTypeObject Font_Type;

/* round a 26.6 pixel coordinate to the nearest larger integer */
#define PIXEL(x) ((((x)+63) & -64)>>6)

static PyObject *
getfont(PyObject* self_, PyObject* args, PyObject* kw)
{
    /* create a font object from a file name and a size (in pixels) */

    FontObject* self;
    int error;

    char* filename;
    int size;
    int index = 0;
    static char* kwlist[] = { "filename", "size", "index", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "si|i", kwlist,
                                     &filename, &size, &index))
        return NULL;

    if (!library) {
        error = FT_Init_FreeType(&library);
        if (error) {
            PyErr_SetString(
                PyExc_IOError,
                "cannot initialize FreeType library"
                );
            return NULL;
        }
    }

    self = PyObject_NEW(FontObject, &Font_Type);
    if (!self)
	return NULL;

    error = FT_New_Face(library, filename, index, &self->face);

    if (!error)
        error = FT_Set_Pixel_Sizes(self->face, 0, size);

    if (error) {
        PyObject_DEL(self);
        PyErr_SetString(PyExc_IOError, "cannot load font");
        return NULL;
    }

    return (PyObject*) self;
}
    
static int
font_getchar(PyObject* string, int index, FT_ULong* char_out)
{
#if PY_VERSION_HEX >= 0x01060000
    if (PyUnicode_Check(string)) {
        Py_UNICODE* p = PyUnicode_AS_UNICODE(string);
        int size = PyUnicode_GET_SIZE(string);
        if (index >= size)
            return 0;
        *char_out = p[index];
        return 1;
    } else
#endif
    if (PyString_Check(string)) {
        unsigned char* p = PyString_AS_STRING(string);
        int size = PyString_GET_SIZE(string);
        if (index >= size)
            return 0;
        *char_out = (unsigned char) p[index];
        return 1;
    }
    return 0;
}

static PyObject*
font_getsize(FontObject* self, PyObject* args)
{
    int i, x;
    FT_ULong ch;

    /* calculate size for a given string */

    PyObject* string;
    if (!PyArg_ParseTuple(args, "O:getsize", &string))
        return NULL;

#if PY_VERSION_HEX >= 0x01060000
    if (!PyUnicode_Check(string) && !PyString_Check(string)) {
#else
    if (!PyString_Check(string)) {
#endif
        PyErr_SetString(PyExc_TypeError, "expected string");
        return NULL;
    }

    for (x = i = 0; font_getchar(string, i, &ch); i++) {
        int index, error;
        index = FT_Get_Char_Index(self->face, ch);
        error = FT_Load_Glyph(self->face, index, FT_LOAD_DEFAULT);
        if (error)
            goto failure;
        x += self->face->glyph->metrics.horiAdvance;
    }

    return Py_BuildValue(
        "ii", PIXEL(x),
        PIXEL(self->face->size->metrics.height)
        );

  failure:
    PyErr_SetString(PyExc_IOError, "cannot load character");
    return NULL;
}

static PyObject*
font_render(FontObject* self, PyObject* args)
{
    int i, x, y;
    Imaging im;
    int index, error, ascender;
    unsigned char *source;
    FT_ULong ch;
    FT_GlyphSlot glyph;

    /* render string into given buffer (the buffer *must* have
       the right size, or this will crash) */
    PyObject* string;
    long id;
    if (!PyArg_ParseTuple(args, "Ol:render", &string, &id))
        return NULL;

#if PY_VERSION_HEX >= 0x01060000
    if (!PyUnicode_Check(string) && !PyString_Check(string)) {
#else
    if (!PyString_Check(string)) {
#endif
        PyErr_SetString(PyExc_TypeError, "expected string");
        return NULL;
    }

    im = (Imaging) id;

    for (x = i = 0; font_getchar(string, i, &ch); i++) {
        index = FT_Get_Char_Index(self->face, ch);
        error = FT_Load_Glyph(self->face, index, FT_LOAD_RENDER);
        if (error)
            goto failure;
        glyph = self->face->glyph;
        source = (unsigned char*) glyph->bitmap.buffer;
        ascender = PIXEL(self->face->size->metrics.ascender);
        for (y = 0; y < glyph->bitmap.rows; y++) {
            int xx = PIXEL(x) + glyph->bitmap_left;
            int yy = y + ascender - glyph->bitmap_top;
            if (yy >= 0 && yy < im->ysize) {
                /* blend this glyph into the buffer */
                int i;
                unsigned char *target = im->image8[yy] + xx;
                for (i = 0; i < glyph->bitmap.width; i++)
                    if (target[i] < source[i])
                        target[i] = source[i];
            }
            source += glyph->bitmap.pitch;
        }
        x += glyph->metrics.horiAdvance;
    }

    Py_INCREF(Py_None);
    return Py_None;

  failure:
    PyErr_SetString(PyExc_IOError, "cannot render character");
    return NULL;
}

static void
font_dealloc(FontObject* self)
{
    FT_Done_Face(self->face);
    PyObject_DEL(self);
}

static PyMethodDef font_methods[] = {
    {"render", (PyCFunction) font_render, METH_VARARGS},
    {"getsize", (PyCFunction) font_getsize, METH_VARARGS},
    {NULL, NULL}
};

static PyObject*  
font_getattr(FontObject* self, char* name)
{
    PyObject* res;

    res = Py_FindMethod(font_methods, (PyObject*) self, name);

    if (res)
        return res;

    PyErr_Clear();

    /* attributes */
    if (!strcmp(name, "family"))
        return PyString_FromString(self->face->family_name);
    if (!strcmp(name, "style"))
        return PyString_FromString(self->face->style_name);

    if (!strcmp(name, "ascent"))
        return PyInt_FromLong(PIXEL(self->face->size->metrics.ascender));
    if (!strcmp(name, "descent"))
        return PyInt_FromLong(-PIXEL(self->face->size->metrics.descender));

    if (!strcmp(name, "glyphs"))
        /* number of glyphs provided by this font */
        return PyInt_FromLong(self->face->num_glyphs);

    PyErr_SetString(PyExc_AttributeError, name);
    return NULL;
}

statichere PyTypeObject Font_Type = {
    PyObject_HEAD_INIT(NULL)
    0, "Font", sizeof(FontObject), 0,
    /* methods */
    (destructor)font_dealloc, /* tp_dealloc */
    0, /* tp_print */
    (getattrfunc)font_getattr, /* tp_getattr */
};

static PyMethodDef _functions[] = {
    {"getfont", (PyCFunction) getfont, METH_VARARGS|METH_KEYWORDS},
    {NULL, NULL}
};

DL_EXPORT(void)
init_imagingft(void)
{
    /* Patch object type */
    Font_Type.ob_type = &PyType_Type;

    Py_InitModule("_imagingft", _functions);
}
