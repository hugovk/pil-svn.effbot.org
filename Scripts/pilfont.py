#
# THIS IS WORK IN PROGRESS
#
# The Python Imaging Library
# $Id$
#
# PIL raster font compiler
#
# history:
# 97-08-25 fl   created
#
# Copyright (c) Fredrik Lundh 1997.  All rights reserved.
#
# See the README file for information on usage and redistribution.
#

VERSION = "0.3"

import glob, os, sys

# drivers
import BdfFontFile
import PcfFontFile

print "PILFONT", VERSION, "-- PIL font compiler."
print "Copyright (c) Fredrik Lundh 1997.  All rights reserved."
print

if len(sys.argv) <= 1:
    print "Usage: pilfont fontfiles..."
    print
    print "Convert given font files to the PIL raster font format."
    print "This version of pilfont supports X BDF and PCF fonts."
    sys.exit(1)

files = []
for f in sys.argv[1:]:
    files = files + glob.glob(f)

for f in files:

    print f + "...",

    try:

        fp = open(f, "rb")

        try:
            p = PcfFontFile.PcfFontFile(fp)
        except SyntaxError:
            fp.seek(0)
            p = BdfFontFile.BdfFontFile(fp)

        p.save(f)

    except (SyntaxError, IOError):
        print "failed"

    else:
        print "OK"
