#
# The Python Imaging Library.
# $Id$
#
# standard filters
#
# History:
#       95-11-27 fl     Created
#
# Copyright (c) Secret Labs AB 1997.
# Copyright (c) Fredrik Lundh 1995.
#
# See the README file for information on usage and redistribution.
#

class _BuiltinFilter:
    def __init__(self, id, name = None):
        self.id = id
        self.name = name

# FIXME: numbers correspond to table in _imagingmodule.c

BLUR = _BuiltinFilter(0, "Blur")
CONTOUR = _BuiltinFilter(1, "Contour")
DETAIL = _BuiltinFilter(2, "Detail")
EDGE_ENHANCE = _BuiltinFilter(3, "Edge-enhance")
EDGE_ENHANCE_MORE = _BuiltinFilter(4, "Edge-enhance More")
EMBOSS = _BuiltinFilter(5, "Emboss")
FIND_EDGES = _BuiltinFilter(6, "Find Edges")
SMOOTH = _BuiltinFilter(7, "Smooth")
SMOOTH_MORE = _BuiltinFilter(8, "Smooth More")
SHARPEN = _BuiltinFilter(9, "Sharpen")

# User defined filters are not supported in release 0.1.  Sorry for that.
