#!/usr/bin/env python
#
# Setup script for PIL
# $Id: //modules/pil/setup.py#14 $
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
ZLIBDIR = "../../kits/zlib-1.1.4"
FREETYPEDIR = "../../kits/freetype-2.0"

# on Windows, the following is used to control how and where to search
# for Tcl/Tk files.  None enables automatic searching; to override, set
# this to a directory name.
TCLROOT = None

from PIL.Image import VERSION

PY_VERSION = sys.version[0] + sys.version[2]

# --------------------------------------------------------------------
# configure imaging module

MODULES = []

INCLUDE_DIRS = ["libImaging"]
LIBRARY_DIRS = ["libImaging"]
LIBRARIES = ["Imaging"]

# Add some standard search spots for MacOSX/darwin
if os.path.exists('/sw/include'):
    INCLUDE_DIRS.append('/sw/include')
if os.path.exists('/sw/lib'):
    LIBRARY_DIRS.append('/sw/lib')

HAVE_LIBJPEG = 0
HAVE_LIBTIFF = 0
HAVE_LIBZ = 0
HAVE_TCLTK = 0

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
    EXTRA_COMPILE_ARGS = None
    EXTRA_LINK_ARGS = None
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
            TCLROOT = None
            print "*** Cannot find Tcl/Tk headers and library files."
            print "*** To build the Tkinter interface, set the TCLROOT"
            print "*** variable in the setup.py file."

        # print "using Tcl/Tk libraries at", TCLROOT
        # print "using Tcl/Tk version", TCL_VERSION

        if TCLROOT:
            version = TCL_VERSION[0] + TCL_VERSION[2]
            INCLUDE_DIRS.append(os.path.join(TCLROOT, "include"))
            LIBRARY_DIRS.append(os.path.join(TCLROOT, "lib"))
            LIBRARIES.extend(["tk" + version, "tcl" + version])
            HAVE_TCLTK = 1
    else:
        tk_framework_found = 0
        if sys.platform == 'darwin':
            # First test for a MacOSX/darwin framework install
            from os.path import join, exists
            framework_dirs = [
                '/System/Library/Frameworks/',
                '/Library/Frameworks',
                join(os.getenv('HOME'), '/Library/Frameworks')
            ]

            # Find the directory that contains the Tcl.framwork and Tk.framework
            # bundles.
            # XXX distutils should support -F!
            for F in framework_dirs:
                # both Tcl.framework and Tk.framework should be present
                for fw in 'Tcl', 'Tk':
                    if not exists(join(F, fw + '.framework')):
                        break
                else:
                    # ok, F is now directory with both frameworks. Continure
                    # building
                    tk_framework_found = 1
                    break
            if tk_framework_found:
                # For 8.4a2, we must add -I options that point inside the Tcl and Tk
                # frameworks. In later release we should hopefully be able to pass
                # the -F option to gcc, which specifies a framework lookup path.
                #
                tk_include_dirs = [
                    join(F, fw + '.framework', H)
                    for fw in 'Tcl', 'Tk'
                    for H in 'Headers', 'Versions/Current/PrivateHeaders'
                ]

                # For 8.4a2, the X11 headers are not included. Rather than include a
                # complicated search, this is a hard-coded path. It could bail out
                # if X11 libs are not found...
                # tk_include_dirs.append('/usr/X11R6/include')
                INCLUDE_DIRS = INCLUDE_DIRS + tk_include_dirs
                frameworks = ['-framework', 'Tcl', '-framework', 'Tk']
                EXTRA_COMPILE_ARGS = frameworks
                EXTRA_LINK_ARGS = frameworks
                HAVE_TCLTK = 1

        if not tk_framework_found:
            # assume the libraries are installed in the default location
            LIBRARIES.extend(["tk" + TCL_VERSION, "tcl" + TCL_VERSION])
            HAVE_TCLTK = 1

    if HAVE_TCLTK:
        MODULES.append(
            Extension(
                "_imagingtk",
                ["_imagingtk.c", "Tk/tkImaging.c"],
                include_dirs=INCLUDE_DIRS,
                library_dirs=LIBRARY_DIRS,
                libraries=LIBRARIES
                )
            )

# --------------------------------------------------------------------
# configure imagingft module

if os.path.isdir(FREETYPEDIR) or os.name == "posix":

    FILES = []
    INCLUDE_DIRS = ["libImaging"]
    LIBRARY_DIRS = []
    LIBRARIES = []
    have_freetype = 1 # Assume we have it, unless proven otherwise

    # use source distribution, if available
    for file in [
        "src/autohint/autohint.c",
        "src/base/ftbase.c",
        #"src/cache/ftcache.c",
        "src/cff/cff.c",
        "src/cid/type1cid.c",
        "src/psaux/psaux.c",
        "src/psnames/psnames.c",
        "src/raster/raster.c",
        "src/sfnt/sfnt.c",
        "src/smooth/smooth.c",
        "src/truetype/truetype.c",
        "src/type1/type1.c",
        "src/winfonts/winfnt.c",
        "src/base/ftsystem.c",
        "src/base/ftinit.c",
        "src/base/ftglyph.c"
        ]:
        file = os.path.join(FREETYPEDIR, file)
        if os.path.isfile(file):
            FILES.append(file)
        else:
            FILES = []
            break

    if FILES:
        INCLUDE_DIRS.append(os.path.join(FREETYPEDIR, "include"))
        INCLUDE_DIRS.append(os.path.join(FREETYPEDIR, "src"))
    elif os.path.isdir("/usr/include/freetype2"):
        # assume that the freetype library is installed in a
        # standard location
        # FIXME: search for libraries
        LIBRARIES.append("freetype")
        INCLUDE_DIRS.append("/usr/include/freetype2")
    elif os.path.isdir("/sw/include/freetype2"):
        # assume that the freetype library is installed in a
        # standard location
        # FIXME: search for libraries
        LIBRARIES.append("freetype")
        INCLUDE_DIRS.append("/sw/include/freetype2")
        LIBRARY_DIRS.append("/sw/lib")
    else:
        have_freetype = 0

    if have_freetype:
        MODULES.append(
            Extension(
                "_imagingft",
                ["_imagingft.c"] + FILES,
                include_dirs=INCLUDE_DIRS,
                library_dirs=LIBRARY_DIRS,
                libraries=LIBRARIES,
                extra_compile_args=EXTRA_COMPILE_ARGS,
                extra_link_args=EXTRA_LINK_ARGS
                )
            )

# build!

if __name__ == "__main__":

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
