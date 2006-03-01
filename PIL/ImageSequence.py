#
# The Python Imaging Library.
# $Id: //modules/pil/PIL/ImageSequence.py#3 $
#
# some sequence support stuff
#
# history:
#       97-02-20 fl     Created
#
# Copyright (c) Secret Labs AB 1997.
# Copyright (c) Fredrik Lundh 1997.
#
# See the README file for information on usage and redistribution.
#

class Iterator:

    """Sequence iterator (use with the for-statement)"""

    def __init__(self, im):
        if not hasattr(im, "seek"):
            raise AttributeError, "im must have seek method"
        self.im = im

    def __getitem__(self, ix):
        try:
            if ix:
                self.im.seek(ix)
            return self.im
        except EOFError:
            raise IndexError # end of sequence
