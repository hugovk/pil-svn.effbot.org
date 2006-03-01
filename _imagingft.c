/*
 * PIL FreeType Driver
 * $Id: _imagingft.c 2025 2004-09-14 08:28:54Z fredrik $
 *
 * a FreeType 2.X driver for PIL
 *
 * history:
 * 2001-02-17 fl  Created (based on old experimental freetype 1.0 code)
 * 2001-04-18 fl  Fixed some egcs compiler nits
 * 2002-11-08 fl  Added unicode support; more font metrics, etc
 * 2003-05-20 fl  Fixed compilation under 1.5.2 and newer non-unicode builds
 * 2003-09-27 fl  Added charmap encoding support
 * 2004-05-15 fl  Fixed compilation for FreeType 2.1.8
 * 2004-09-10 fl  Added support for monochrome bitmaps
 *
 * Copyright (c) 1998-2004 by Secret Labs AB
 */

#include "Python.h"
#include "Imaging.h"

#ifndef USE_FREETYPE_2_0
/* undef/comment out to use freetype 2.0 */
#define USE_FREETYPE_2_1
#endif

#if defined(USE_FREETYPE_2_1)
/* freetype 2.1 and newer */
#include <ft2build.h>
#include FT_FREETYPE_H
#else
/* freetype 2.0 */
#include <freetype/freetype.h>
#endif

#if defined(PY_VERSION_HEX) && PY_VERSION_HEX < 0x01060000
#define PyObject_DEL(op) PyMem_DEL((op))
#endif

#if defined(PY_VERSION_HEX) && PY_VERSION_HEX >= 0x01060000
#if PY_VERSION_HEX  < 0x02020000 || defined(Py_USING_UNICODE)
/* defining this enables unicode support (default under 1.6a1 and later) */
#define HAVE_UNICODE
#endif
#endif

#ifndef FT_LOAD_TARGET_MONO
#define FT_LOAD_TARGET_MONO  FT_LOAD_MONOCHROME
#endif

/* -------------------------------------------------------------------- */
/* error table */

#undef FTERRORS_H
#undef __FTERRORS_H__

#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST  {
#define FT_ERROR_END_LIST    { 0, 0 } };

struct {
    int code;
    const char* message;
} ft_errors[] =

#include <freetype/fterrors.h>

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

static PyObject*
geterror(int code)
{
    int i;

    for (i = 0; ft_errors[i].message; i++)
        if (ft_errors[i].code == code) {
            PyErr_SetString(PyExc_IOError, ft_errors[i].message);
            return NULL;
        }

    PyErr_SetString(PyExc_IOError, "unknown freetype error");
    return NULL;
}

static PyObject*
getfont(PyObject* self_, PyObject* args, PyObject* kw)
{
    /* create a font object from a file name and a size (in pixels) */

    FontObject* self;
    int error;

    char* filename;
    int size;
    int index = 0;
    unsigned char* encoding = NULL;
    static char* kwlist[] = {
        "filename", "size", "index", "encoding", NULL
    };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "si|is", kwlist,
                                     &filename, &size, &index, &encoding))
        return NULL;

    if (!library && FT_Init_FreeType(&library)) {
        PyErr_SetString(
            PyExc_IOError,
            "cannot initialize FreeType library"
            );
        return NULL;
    }

    self = PyObject_NEW(FontObject, &Font_Type);
    if (!self)
	return NULL;

    error = FT_New_Face(library, filename, index, &self->face);

    if (!error)
        error = FT_Set_Pixel_Sizes(self->face, 0, size);

    if (!error && encoding && strlen((char*) encoding) == 4) {
        FT_Encoding encoding_tag = FT_MAKE_TAG(
            encoding[0], encoding[1], encoding[2], encoding[3]
            );
        error = FT_Select_Charmap(self->face, encoding_tag);
    }

    if (error) {
        PyObject_DEL(self);
        return geterror(error);
    }

    return (PyObject*) self;
}
    
static int
font_getchar(PyObject* string, int index, FT_ULong* char_out)
{
#if defined(HAVE_UNICODE)
    if (PyUnicode_Check(string)) {
        Py_UNICODE* p = PyUnicode_AS_UNICODE(string);
        int size = PyUnicode_GET_SIZE(string);
        if (index >= size)
            return 0;
        *char_out = p[index];
        return 1;
    }
#endif
    if (PyString_Check(string)) {
        unsigned char* p = (unsigned char*) PyString_AS_STRING(string);
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

#if defined(HAVE_UNICODE)
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
            return geterror(error);
        x += self->face->glyph->metrics.horiAdvance;
    }

    return Py_BuildValue(
        "ii", PIXEL(x),
        PIXEL(self->face->size->metrics.height)
        );
}

static PyObject*
font_render(FontObject* self, PyObject* args)
{
    int i, x, y;
    Imaging im;
    int index, error, ascender;
    int load_flags;
    unsigned char *source;
    
    FT_ULong ch;
    FT_GlyphSlot glyph;

    /* render string into given buffer (the buffer *must* have
       the right size, or this will crash) */
    PyObject* string;
    long id;
    int mask = 0;
    if (!PyArg_ParseTuple(args, "Ol|i:render", &string, &id, &mask))
        return NULL;

#if defined(HAVE_UNICODE)
    if (!PyUnicode_Check(string) && !PyString_Check(string)) {
#else
    if (!PyString_Check(string)) {
#endif
        PyErr_SetString(PyExc_TypeError, "expected string");
        return NULL;
    }

    im = (Imaging) id;

    load_flags = FT_LOAD_RENDER;
    if (mask)
        load_flags |= FT_LOAD_TARGET_MONO;

    for (x = i = 0; font_getchar(string, i, &ch); i++) {
        index = FT_Get_Char_Index(self->face, ch);
        error = FT_Load_Glyph(self->face, index, load_flags);
        if (error)
            return geterror(error);
        glyph = self->face->glyph;
        if (mask) {
            /* use monochrome mask (on palette images, etc) */
            source = (unsigned char*) glyph->bitmap.buffer;
            ascender = PIXEL(self->face->size->metrics.ascender);
            for (y = 0; y < glyph->bitmap.rows; y++) {
                int xx = PIXEL(x) + glyph->bitmap_left;
                int yy = y + ascender - glyph->bitmap_top;
                if (yy >= 0 && yy < im->ysize) {
                    /* blend this glyph into the buffer */
                    int i, j, m;
                    unsigned char *target = im->image8[yy] + xx;
                    m = 128;
                    for (i = j = 0; j < glyph->bitmap.width; j++) {
                        if (source[i] & m)
                            target[j] = 255;
                        if (!(m >>= 1)) {
                            m = 128;
                            i++;
                        }
                    }
                }
                source += glyph->bitmap.pitch;
            }
        } else {
            /* use antialiased rendering */
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
        }
        x += glyph->metrics.horiAdvance;
    }

    Py_INCREF(Py_None);
    return Py_None;
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
