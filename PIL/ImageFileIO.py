#
# The Python Imaging Library.
# $Id$
#
# kludge to get basic ImageFileIO functionality
#
# History:
# 98-08-06 fl   Recreated
#
# Copyright (c) Secret Labs AB 1998.
#
# See the README file for information on usage and redistribution.
#

from StringIO import StringIO

class ImageFileIO(StringIO):
    def __init__(self, fp):
        data = fp.read()
        StringIO.__init__(self, data)

if __name__ == "__main__":

    import Image
    fp = open("/images/clenna.im", "rb")
    im = Image.open(ImageFileIO(fp))
    im.load() # make sure we can read the raster data
    print im.mode, im.size
