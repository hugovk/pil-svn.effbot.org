/***********************************************************
(C) Copyright 1998,1999 A.M. Kuchling.  All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of A.M. Kuchling not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

A.M. KUCHLING DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL A.M. KUCHLING BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* SaneDev objects */

#include "Python.h"
#include "Imaging.h"
#include <sane/sane.h>

static PyObject *ErrorObject;

typedef struct {
	PyObject_HEAD
	SANE_Handle h;
} SaneDevObject;

/* Raise a SANE exception */
PyObject * 
PySane_Error(st)
     SANE_Status st;
{
  const char *string;
  
  if (st==SANE_STATUS_GOOD) {Py_INCREF(Py_None); return (Py_None);}
  string=sane_strstatus(st);
  PyErr_SetString(ErrorObject, string);
  return NULL;
}

staticforward PyTypeObject SaneDev_Type;

#define SaneDevObject_Check(v)	((v)->ob_type == &SaneDev_Type)

static SaneDevObject *
newSaneDevObject(arg)
	PyObject *arg;
{
	SaneDevObject *self;
	self = PyObject_NEW(SaneDevObject, &SaneDev_Type);
	if (self == NULL)
		return NULL;
	self->h=NULL;
	return self;
}

/* SaneDev methods */

static void
SaneDev_dealloc(self)
	SaneDevObject *self;
{
  if (self->h) sane_close(self->h);
  self->h=NULL;
  PyMem_DEL(self);
}

static PyObject *
SaneDev_close(self, args)
     SaneDevObject *self;
     PyObject *args;
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  if (self->h) sane_close(self->h);
  self->h=NULL;
  Py_INCREF(Py_None);
  return (Py_None);
}

static PyObject *
SaneDev_get_parameters(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  SANE_Status st;
  SANE_Parameters p;
  char *format="unknown format";
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }
  st=sane_get_parameters(self->h, &p);
  if (st) return PySane_Error(st);
  switch (p.format)
    {
    case(SANE_FRAME_GRAY): format="L"; break;
    case(SANE_FRAME_RGB): format="RGB"; break;
    case(SANE_FRAME_RED): format="R"; break;
    case(SANE_FRAME_GREEN): format="G"; break;
    case(SANE_FRAME_BLUE): format="B"; break;
    }
  
  return Py_BuildValue("si(ii)ii", format, p.last_frame, p.pixels_per_line, p.lines, 
		       p.depth, p.bytes_per_line);
}


static PyObject *
SaneDev_fileno(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  SANE_Status st;
  SANE_Int fd;
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }
  st=sane_get_select_fd(self->h, &fd);
  if (st) return PySane_Error(st);
  return PyInt_FromLong(fd);
}

static PyObject *
SaneDev_start(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  SANE_Status st;
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }

  Py_BEGIN_ALLOW_THREADS
  st=sane_start(self->h);
  Py_END_ALLOW_THREADS

  if (st) return PySane_Error(st);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
SaneDev_cancel(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }

  Py_BEGIN_ALLOW_THREADS
  sane_cancel(self->h);
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
SaneDev_get_options(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  const SANE_Option_Descriptor *d;
  PyObject *list, *value;
  int i=1;
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }
  if (!(list = PyList_New(0)))
	    return NULL;

  do 
    {
      d=sane_get_option_descriptor(self->h, i);
      if (d!=NULL) 
	{
	  PyObject *constraint;
	  int j;
	  
	  switch (d->constraint_type)
	    {
	    case(SANE_CONSTRAINT_NONE): Py_INCREF(Py_None); constraint=Py_None; break;
	    case(SANE_CONSTRAINT_RANGE): 
	      constraint=Py_BuildValue("iii", d->constraint.range->min, d->constraint.range->max, d->constraint.range->quant);
	      break;
	    case(SANE_CONSTRAINT_WORD_LIST): 
	      constraint=PyList_New(d->constraint.word_list[0]);
	      for (j=1; j<=d->constraint.word_list[0]; j++)
		PyList_SetItem(constraint, j-1, PyInt_FromLong(d->constraint.word_list[j]));
	      break;
	    case(SANE_CONSTRAINT_STRING_LIST): 
	      constraint=PyList_New(0);
	      for(j=0; d->constraint.string_list[j]!=NULL; j++)
		PyList_Append(constraint, PyString_FromString(d->constraint.string_list[j]) );
	      break;
	    }
	  value=Py_BuildValue("isssiiiiO", i, d->name, d->title, d->desc, d->type,
			      d->unit, d->size, d->cap, constraint);
	  PyList_Append(list, value);
	}
      i++;
    } while (d!=NULL);
  return list;
}

static PyObject *
SaneDev_get_option(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  SANE_Status st;
  const SANE_Option_Descriptor *d;
  SANE_Int i;
  PyObject *value;
  int n;
  void *v;
  
  if (!PyArg_ParseTuple(args, "i", &n))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }
  d=sane_get_option_descriptor(self->h, n);
  v=malloc(d->size+1);
  st=sane_control_option(self->h, n, SANE_ACTION_GET_VALUE,
			 v, &i);

  if (st) {free(v); return PySane_Error(st);}
  
  switch(d->type)
    {
    case(SANE_TYPE_BOOL):
    case(SANE_TYPE_INT):
      value=Py_BuildValue("ii", i, *( (SANE_Int*)v) );
      break;
    case(SANE_TYPE_FIXED):
      value=Py_BuildValue("id", i, SANE_UNFIX((*((SANE_Fixed*)v))) );
      break;
    case(SANE_TYPE_STRING):
      value=Py_BuildValue("is", i, v);
      break;
    case(SANE_TYPE_BUTTON):
    case(SANE_TYPE_GROUP):
      value=Py_BuildValue("iO", i, Py_None);
      break;
    }
  
  free(v);
  return value;
}

static PyObject *
SaneDev_set_option(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  SANE_Status st;
  const SANE_Option_Descriptor *d;
  SANE_Int i;
  PyObject *value;
  int n;
  void *v;
  
  if (!PyArg_ParseTuple(args, "iO", &n, &value))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }
  d=sane_get_option_descriptor(self->h, n);
  v=malloc(d->size+1);

  switch(d->type)
    {
    case(SANE_TYPE_BOOL):
      if (!PyInt_Check(value)) 
	{
	  PyErr_SetString(PyExc_TypeError, "SANE_BOOL requires an integer");
	  free(v);
	  return NULL;
	}
	/* fall through */
    case(SANE_TYPE_INT):
      if (!PyInt_Check(value)) 
	{
	  PyErr_SetString(PyExc_TypeError, "SANE_INT requires an integer");
	  free(v);
	  return NULL;
	}
      *( (SANE_Int*)v) = PyInt_AsLong(value);
      break;
    case(SANE_TYPE_FIXED):
      if (!PyFloat_Check(value)) 
	{
	  PyErr_SetString(PyExc_TypeError, "SANE_FIXED requires a floating point number");
	  free(v);
	  return NULL;
	}
      *( (SANE_Fixed*)v) = SANE_FIX(PyFloat_AsDouble(value));
      break;
    case(SANE_TYPE_STRING):
      if (!PyString_Check(value)) 
	{
	  PyErr_SetString(PyExc_TypeError, "SANE_STRING requires a string");
	  free(v);
	  return NULL;
	}
      strcpy(v, PyString_AsString(value));
      break;
    case(SANE_TYPE_BUTTON): 
    case(SANE_TYPE_GROUP):
      break;
    }
  
  st=sane_control_option(self->h, n, SANE_ACTION_SET_VALUE,
			 v, &i);
  if (st) {free(v); return PySane_Error(st);}
  
  free(v);
  return Py_BuildValue("i", i);
}

static PyObject *
SaneDev_set_auto_option(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  SANE_Status st;
  const SANE_Option_Descriptor *d;
  SANE_Int i;
  int n;
  
  if (!PyArg_ParseTuple(args, "i", &n))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }
  d=sane_get_option_descriptor(self->h, n);
  st=sane_control_option(self->h, n, SANE_ACTION_SET_AUTO,
			 NULL, &i);
  if (st) {return PySane_Error(st);}
  
  return Py_BuildValue("i", i);
 }

static PyObject *
SaneDev_snap(self, args)
	SaneDevObject *self;
	PyObject *args;
{
  SANE_Status st; 
   /* The buffer should be a multiple of 3 in size, so each sane_read
      operation will return an integral number of RGB triples. */
  SANE_Byte buffer[8192*3];  
  SANE_Int len;
  Imaging im;
  SANE_Parameters p;
  char *format="unknown format";
  int px, py, total=0;
  long L;
  
  if (!PyArg_ParseTuple(args, "i", &L))
    return NULL;
  if (self->h==NULL)
    {
      PyErr_SetString(ErrorObject, "SaneDev object is closed");
      return NULL;
    }
  im=(Imaging)L;

 do_frame:
  
  st = sane_get_parameters(self->h, &p);
  if (st != SANE_STATUS_GOOD) 
    {
      return PySane_Error(st);
    }

  st=SANE_STATUS_GOOD; px=py=0;
  while (st!=SANE_STATUS_EOF)
    {
      st=sane_read(self->h, buffer, sizeof(buffer), &len);
      if (st && (st!=SANE_STATUS_EOF)) return PySane_Error(st);
      total += len;
      if (st==SANE_STATUS_GOOD)
	{
	  switch(p.format)
	    {
	    case SANE_FRAME_RED:
	    case SANE_FRAME_BLUE:
	    case SANE_FRAME_GREEN:
	    {
	      /* Handle a single band of colour */
	      int i;
	      int offset = 8 * (p.format - SANE_FRAME_RED);
	      for (i=0; i<len && py <im->ysize; i++)
		{
		  im->image32[py][px] = (im->image32[py][px] & ~(0xFF << offset)) 
		                        | (buffer[i] << offset) 
		                        | (0xFF << 24);
		  if (++px >= (int) im->xsize)
		    {px = 0; py++;}
		}
	      break;
	    }
	    case SANE_FRAME_GRAY:
	    {
	      /* Handle some sort of 8-bit code */
	      int i;
	      for (i=0; i<len && py <im->ysize; i++)
		{
		  im->image8[py][px]=buffer[i];
		  if (++px >= (int) im->xsize)
		    {px = 0; py++;}
		}
	      break;
	    }
	    case SANE_FRAME_RGB:
	    {
	      /* Handle 24-bit colour */
	      int i;
	      for (i=0; i<len && py <im->ysize; i+=3)
		{
		  im->image32[py][px]=buffer[i] + 
		                      (buffer[i+1]<<8) +
		                      (buffer[i+2]<<16);
		  if (++px >= (int) im->xsize)
		    {px = 0; py++;}
		}
	      break;
	    }
	    default:
	      PyErr_Format(ErrorObject, "Unknown frame type: %i", p.format);
	      return NULL;
	      break;
	    }
	}
    }
  
  if (!p.last_frame)   goto do_frame;
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef SaneDev_methods[] = {
	{"get_parameters",	(PyCFunction)SaneDev_get_parameters,	1},

	{"get_options",	(PyCFunction)SaneDev_get_options,	1},
	{"get_option",	(PyCFunction)SaneDev_get_option,	1},
	{"set_option",	(PyCFunction)SaneDev_set_option,	1},
	{"set_auto_option",	(PyCFunction)SaneDev_set_auto_option,	1},

	{"start",	(PyCFunction)SaneDev_start,	1},
	{"cancel",	(PyCFunction)SaneDev_cancel,	1},
	{"snap",	(PyCFunction)SaneDev_snap,	1},
	{"fileno",	(PyCFunction)SaneDev_fileno,	1},
 	{"close",	(PyCFunction)SaneDev_close,	1},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
SaneDev_getattr(self, name)
	SaneDevObject *self;
	char *name;
{
#if 0
	if (self->x_attr != NULL) {
		PyObject *v = PyDict_GetItemString(self->x_attr, name);
		if (v != NULL) {
			Py_INCREF(v);
			return v;
		}
	}
#endif
	return Py_FindMethod(SaneDev_methods, (PyObject *)self, name);
}

static int
SaneDev_setattr(self, name, v)
	SaneDevObject *self;
	char *name;
	PyObject *v;
{
#if 0
	if (self->x_attr == NULL) {
		self->x_attr = PyDict_New();
		if (self->x_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(self->x_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
			        "delete non-existing SaneDev attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(self->x_attr, name, v);
#endif
}

staticforward PyTypeObject SaneDev_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"SaneDev",			/*tp_name*/
	sizeof(SaneDevObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)SaneDev_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)SaneDev_getattr, /*tp_getattr*/
	(setattrfunc)SaneDev_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

/* --------------------------------------------------------------------- */

static PyObject *
PySane_init(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
  SANE_Status st;
  SANE_Int version;
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  /* XXX Authorization is not yet supported */
  st=sane_init(&version, NULL); 
  if (st) return PySane_Error(st);
  return Py_BuildValue("iiii", version, SANE_VERSION_MAJOR(version),
		       SANE_VERSION_MINOR(version), SANE_VERSION_BUILD(version));
}

static PyObject *
PySane_exit(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  sane_exit();
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PySane_get_devices(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
  const SANE_Device **devlist;
  SANE_Device *dev;
  SANE_Status st;
  PyObject *list;
  int local_only, i;
  
  if (!PyArg_ParseTuple(args, "|i", &local_only))
    {
      return NULL;
    }
  
  st=sane_get_devices(&devlist, local_only);
  if (st) return PySane_Error(st);
  if (!(list = PyList_New(0)))
	    return NULL;
  for(i=0; devlist[i]!=NULL; i++)
    {
      dev=devlist[i];
      PyList_Append(list, Py_BuildValue("ssss", dev->name, dev->vendor, dev->model, dev->type));
    }
  
  return list;
}

/* Function returning new SaneDev object */

static PyObject *
PySane_open(self, args)
        PyObject *self;
	PyObject *args;
{
	SaneDevObject *rv;
	SANE_Status st;
	char *name;

	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	rv = newSaneDevObject();
	if ( rv == NULL )
	    return NULL;
	st = sane_open(name, &(rv->h));
	if (st) 
	  {
	    Py_DECREF(rv);
	    return PySane_Error(st);
	  }
	return (PyObject *)rv;
}

static PyObject *
PySane_OPTION_IS_ACTIVE(self, args)
        PyObject *self;
	PyObject *args;
{
  SANE_Int cap;
  long lg;
  
  if (!PyArg_ParseTuple(args, "i", &lg))
    return NULL;
  cap=lg;
  return PyInt_FromLong( SANE_OPTION_IS_ACTIVE(cap));
}

static PyObject *
PySane_OPTION_IS_SETTABLE(self, args)
        PyObject *self;
	PyObject *args;
{
  SANE_Int cap;
  long lg;
  
  if (!PyArg_ParseTuple(args, "i", &lg))
    return NULL;
  cap=lg;
  return PyInt_FromLong( SANE_OPTION_IS_SETTABLE(cap));
}


/* List of functions defined in the module */

static PyMethodDef PySane_methods[] = {
	{"init",	PySane_init,		1},
	{"exit",	PySane_exit,		1},
	{"get_devices",	PySane_get_devices,	1},
	{"_open",	PySane_open,	1},
	{"OPTION_IS_ACTIVE",	PySane_OPTION_IS_ACTIVE,	1},
	{"OPTION_IS_SETTABLE",	PySane_OPTION_IS_SETTABLE,	1},
	{NULL,		NULL}		/* sentinel */
};


static void
insint(d, name, value)
     PyObject *d;
     char *name;
     int value;
{
	PyObject *v = PyInt_FromLong((long) value);
	if (!v || PyDict_SetItemString(d, name, v))
		Py_FatalError("can't initialize sane module");

	Py_DECREF(v);
}

void
init_sane()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule("_sane", PySane_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("_sane.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	insint(d, "INFO_INEXACT", SANE_INFO_INEXACT);
	insint(d, "INFO_RELOAD_OPTIONS", SANE_INFO_RELOAD_OPTIONS);
	insint(d, "RELOAD_PARAMS", SANE_INFO_RELOAD_PARAMS);

	insint(d, "FRAME_GRAY", SANE_FRAME_GRAY);
	insint(d, "FRAME_RGB", SANE_FRAME_RGB);
	insint(d, "FRAME_RED", SANE_FRAME_RED);
	insint(d, "FRAME_GREEN", SANE_FRAME_GREEN);
	insint(d, "FRAME_BLUE", SANE_FRAME_BLUE);

	insint(d, "CONSTRAINT_NONE", SANE_CONSTRAINT_NONE);
	insint(d, "CONSTRAINT_RANGE", SANE_CONSTRAINT_RANGE);
	insint(d, "CONSTRAINT_WORD_LIST", SANE_CONSTRAINT_WORD_LIST);
	insint(d, "CONSTRAINT_STRING_LIST", SANE_CONSTRAINT_STRING_LIST);

	insint(d, "TYPE_BOOL", SANE_TYPE_BOOL);
	insint(d, "TYPE_INT", SANE_TYPE_INT);
	insint(d, "TYPE_FIXED", SANE_TYPE_FIXED);
	insint(d, "TYPE_STRING", SANE_TYPE_STRING);
	insint(d, "TYPE_BUTTON", SANE_TYPE_BUTTON);
	insint(d, "TYPE_GROUP", SANE_TYPE_GROUP);

	insint(d, "UNIT_NONE", SANE_UNIT_NONE);
	insint(d, "UNIT_PIXEL", SANE_UNIT_PIXEL);
	insint(d, "UNIT_BIT", SANE_UNIT_BIT);
	insint(d, "UNIT_MM", SANE_UNIT_MM);
	insint(d, "UNIT_DPI", SANE_UNIT_DPI);
	insint(d, "UNIT_PERCENT", SANE_UNIT_PERCENT);

	insint(d, "CAP_SOFT_SELECT", SANE_CAP_SOFT_SELECT);
	insint(d, "CAP_HARD_SELECT", SANE_CAP_HARD_SELECT);
	insint(d, "CAP_SOFT_DETECT", SANE_CAP_SOFT_DETECT);
	insint(d, "CAP_EMULATED", SANE_CAP_EMULATED);
	insint(d, "CAP_AUTOMATIC", SANE_CAP_AUTOMATIC);
	insint(d, "CAP_INACTIVE", SANE_CAP_INACTIVE);
	insint(d, "CAP_ADVANCED", SANE_CAP_ADVANCED);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module _sane");
}

