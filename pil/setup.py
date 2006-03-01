#!/usr/bin/env python
#
# Setup script for PIL
# $Id: //modules/pil/setup.py#7 $
#
# Usage: python setup.py install
#
# before running this script, build the libImaging directory
# and all support libraries (the JPEG library, ZLIB, etc).
#

from distutils.core import setup, Extension

import os, sys, re

# --------------------------------------------------------------------
# configuration

NAME = "PIL"
DESCRIPTION = "Python Imaging Library"
AUTHOR = "Secret Labs AB / PythonWare", "info@pythonware.com"
HOMEPAGE = "http://www.pythonware.com/products/pil"

# on windows, the build script expects to find both library files and
# include files in the directories below.  tweak as necessary.
JPEGDIR = "../../kits/jpeg-6b"
ZLIBDIR = "../../kits/zlib-1.1.3"

# on windows, the following is used to control how and where to search
# for Tcl/Tk files.  None enables automatic searching; to override, set
# this to a directory name
TCLROOT = None

from PIL.Image import VERSION

PY_VERSION = sys.version[0] + sys.version[2]

# --------------------------------------------------------------------
# configure imaging module

MODULES = []

INCLUDE_DIRS = ["libImaging"]
LIBRARY_DIRS = ["libImaging"]
LIBRARIES = ["Imaging"]

HAVE_LIBJPEG = 0
HAVE_LIBTIFF = 0
HAVE_LIBZ = 0

# parse ImConfig.h to figure out what external libraries we're using
for line in open(os.path.join("libImaging", "ImConfig.h")).readlines():
    m = re.match("#define\s+HAVE_LIB([A-Z]+)", line)
    if m:
        lib = m.group(1)
        if lib == "JPEG":
            HAVE_LIBJPEG = 1
            if sys.platform == "win32":
                LIBRARIES.append("jpeg")
                INCLUDE_DIRS.append(JPEGDIR)
                LIBRARY_DIRS.append(JPEGDIR)
            else:
                LIBRARIES.append("jpeg")
        elif lib == "TIFF":
            HAVE_LIBTIFF = 1
            LIBRARIES.append("tiff")
        elif lib == "Z":
            HAVE_LIBZ = 1
            if sys.platform == "win32":
                LIBRARIES.append("zlib")
                INCLUDE_DIRS.append(ZLIBDIR)
                LIBRARY_DIRS.append(ZLIBDIR)
            else:
                LIBRARIES.append("z")

if sys.platform == "win32":
    # standard windows libraries
    LIBRARIES.extend(["kernel32", "user32", "gdi32"])

MODULES.append(
    Extension(
        "_imaging",
        ["_imaging.c", "decode.c", "encode.c", "map.c", "display.c",
         "outline.c", "path.c"],
        include_dirs=INCLUDE_DIRS,
        library_dirs=LIBRARY_DIRS,
        libraries=LIBRARIES
        )
    )

# security check

if HAVE_LIBZ:
    # look for old, unsafe version of zlib
    # note: this only finds zlib.h if ZLIBDIR is properly set
    zlibfile = os.path.join(ZLIBDIR, "zlib.h")
    if os.path.isfile(zlibfile):
        for line in open(zlibfile).readlines():
            m = re.match('#define\s+ZLIB_VERSION\s+"([^"]*)"', line)
            if m:
                if m.group(1) < "1.1.4":
                    print
                    print "*** Warning: zlib", m.group(1),
                    print "may contain a security vulnerability."
                    print "*** Consider upgrading to zlib 1.1.4 or newer."
                    print "*** See:",
                    print "http://www.gzip.org/zlib/advisory-2002-03-11.txt"
                    print
                break

# --------------------------------------------------------------------
# configure imagingtk module

try:
    import _tkinter
    TCL_VERSION = _tkinter.TCL_VERSION[:3]
except (ImportError, AttributeError):
    pass
else:
    INCLUDE_DIRS = ["libImaging"]
    LIBRARY_DIRS = ["libImaging"]
    LIBRARIES = ["Imaging"]
    if sys.platform == "win32":
        # locate tcl/tk on windows
        if TCLROOT:
            path = [TCLROOT]
        else:
            path = [
                os.path.join("/py" + PY_VERSION, "Tcl"),
                os.path.join(os.environ.get("ProgramFiles", ""), "Tcl"),
                # FIXME: add more directories here?
                ]
        for root in path:
            TCLROOT = os.path.abspath(root)
            if os.path.isfile(os.path.join(TCLROOT, "include", "tk.h")):
                break
        else:
            print "*** cannot find Tcl/Tk headers and library files"
            print "    change the TCLROOT variable in the setup.py file"
            sys.exit(1)

        # print "using Tcl/Tk libraries at", TCLROOT
        # print "using Tcl/Tk version", TCL_VERSION

        version = TCL_VERSION[0] + TCL_VERSION[2]
        INCLUDE_DIRS.append(os.path.join(TCLROOT, "include"))
        LIBRARY_DIRS.append(os.path.join(TCLROOT, "lib"))
        LIBRARIES.extend(["tk" + version, "tcl" + version])
    else:
        # assume the libraries are installed in the default location
        LIBRARIES.extend(["tk" + TCL_VERSION, "tcl" + TCL_VERSION])

    MODULES.append(
        Extension(
            "_imagingtk",
            ["_imagingtk.c", "Tk/tkImaging.c"],
            include_dirs=INCLUDE_DIRS,
            library_dirs=LIBRARY_DIRS,
            libraries=LIBRARIES
            )
        )

# build!

setup(
    name=NAME,
    version=VERSION,
    author=AUTHOR[0],
    author_email=AUTHOR[1],
    description=DESCRIPTION,
    url=HOMEPAGE,
    packages=[""],
    extra_path = "PIL",
    package_dir={"": "PIL"},
    ext_modules = MODULES,
    )
