#
# The Python Imaging Library.
# $Id$
#
# a class to read from a container file
#
# History:
# 1995-06-18 fl     Created
# 1995-09-07 fl     Added readline(), readlines()
#
# Copyright (c) 1997-2001 by Secret Labs AB
# Copyright (c) 1995 by Fredrik Lundh
#
# See the README file for information on usage and redistribution.
#

# --------------------------------------------------------------------
# Return a restricted file object allowing a user to read and
# seek/tell an individual file within a container file (for example
# a TAR file).

class ContainerIO:

    def __init__(self, fh, offset, length):
        self.fh = fh
        self.pos = 0
        self.offset = offset
        self.length = length
        self.fh.seek(offset)

    def isatty(self):
        return 0

    def seek(self, offset, mode = 0):
        if mode == 1:
            self.pos = self.pos + offset
        elif mode == 2:
            self.pos = self.length + offset
        else:
            self.pos = offset
        # clamp
        self.pos = max(0, min(self.pos, self.length))
        self.fh.seek(self.offset + self.pos)

    def tell(self):
        return self.pos

    def read(self, n = 0):
        if n:
            n = min(n, self.length - self.pos)
        else:
            n = self.length - self.pos
        if not n: # EOF
            return ""
        self.pos = self.pos + n
        return self.fh.read(n)

    def readline(self):
        s = ""
        while 1:
            c = self.read(1)
            if not c:
                break
            s = s + c
            if c == "\n":
                break
        return s

    def readlines(self):
        l = []
        while 1:
            s = self.readline()
            if not s:
                break
            l.append(s)
        return l
