# sane.py
#
# Python wrapper on top of the _sane module, which is in turn a very
# thin wrapper on top of the SANE library.  For a complete understanding
# of SANE, consult the documentation at the SANE home page:
# http://www.mostang.com/sane/ .

__revision__ = "$Id: sane.py 20716 2003-02-11 17:51:50Z akuchlin $"

from PIL import Image

import _sane
from _sane import *

class Option:
    """Class representing a SANE option.
    Attributes:
    index -- number from 0 to n, giving the option number
    name -- a string uniquely identifying the option
    title -- single-line string containing a title for the option
    desc -- a long string describing the option; useful as a help message
    type -- type of this option.  Possible values: TYPE_BOOL,
            TYPE_INT, TYPE_STRING, and so forth.
    unit -- units of this option.  Possible values: UNIT_NONE,
            UNIT_PIXEL, etc.
    size -- size of the value in bytes
    cap -- capabilities available; CAP_EMULATED, CAP_SOFT_SELECT, etc.
    constraint -- constraint on values.  Possible values:
        None : No constraint
        (min,max,step)  Integer values, from min to max, stepping by
        list of integers or strings: only the listed values are allowed
    """

    def __init__(self, args):
        import string
        self.index, self.name = args[0], args[1]
        self.title, self.desc = args[2], args[3]
        self.type, self.unit  = args[4], args[5]
        self.size, self.cap   = args[6], args[7]
        self.constraint = args[8]
        def f(x):
            if x=='-': return '_'
            else: return x
        if type(self.name)!=type(''): self.py_name=str(self.name)
        else: self.py_name=string.join(map(f, self.name), '')

    def is_active(self):
        return _sane.OPTION_IS_ACTIVE(self.cap)
    def is_settable(self):
        return _sane.OPTION_IS_SETTABLE(self.cap)

class SaneDev:
    """Class representing a SANE device.
    Methods:
    start() -- initiate a scan, using the current settings
    snap() -- snap a picture, returning an Image object
    cancel() -- cancel an in-progress scanning operation
    fileno() -- return the file descriptor for the scanner (handy for select)

    Also available, but rather low-level:
    get_parameters() -- get the current parameter settings of the device
    get_options() -- return a list of tuples describing all the options.
                     Use .opt and .optlist attributes instead.
    Attributes:
    optlist -- list of option names
    opt -- a dictionary mapping option names to Option objects.

    You can also access an option name to retrieve its value, and to
    set it.  For example, if one option has a .name attribute of
    imagemode, and scanner is a SaneDev object, you can do:
         print scanner.imagemode
         scanner.imagemode = 'Full frame'
         scanner.opt['imagemode'] returns the corresponding Option object.
    """
    def __init__(self, devname):
        d=self.__dict__
        d['dev']=_sane._open(devname)
        d['opt']={}

        optlist=d['dev'].get_options()
        for t in optlist:
            o=Option(t)
            if o.type!=TYPE_GROUP:
                d['opt'][o.py_name]=o

    def __setattr__(self, key, value):
        dev=self.__dict__['dev']
        optdict=self.__dict__['opt']
        if not optdict.has_key(key):
            self.__dict__[key]=value ; return
        opt=optdict[key]
        if opt.type==TYPE_GROUP:
            raise AttributeError, "Groups can't be set: "+key
        if not _sane.OPTION_IS_ACTIVE(opt.cap):
            raise AttributeError, 'Inactive option: '+key
        if not _sane.OPTION_IS_SETTABLE(opt.cap):
            raise AttributeError, "Option can't be set by software: "+key

        self.last_opt = dev.set_option(opt.index, value)

    def __getattr__(self, key):
        dev=self.__dict__['dev']
        optdict=self.__dict__['opt']
        if key=='optlist':
            return self.opt.keys()
        if not optdict.has_key(key):
            raise AttributeError, 'No such attribute: '+key
        opt=optdict[key]
        if opt.type==TYPE_BUTTON:
            raise AttributeError, "Buttons don't have values: "+key
        if opt.type==TYPE_GROUP:
            raise AttributeError, "Groups don't have values: "+key
        if not _sane.OPTION_IS_ACTIVE(opt.cap):
            raise AttributeError, 'Inactive option: '+key
        self.last_opt, value = dev.get_option(opt.index)
        return value

    def get_parameters(self):
        "Return a tuple holding all the current device settings"
        return self.dev.get_parameters()

    def get_options(self):
        "Return a list of tuples describing all the available options"
        return self.dev.get_options()

    def start(self):
        "Initiate a scanning operation"
        return self.dev.start()

    def cancel(self):
        "Cancel an in-progress scanning operation"
        return self.dev.cancel()

    def snap(self):
        "Snap a picture, returning a PIL image object with the results"
        format, last_frame, (xsize, ysize), depth, bytes_per_line = self.get_parameters()
        if (format=='G') or (format=='R') or (format=='B'): format='L'
        im=Image.new(format, (xsize,ysize))
        self.dev.snap( im.im.id )
        return im

    def fileno(self):
        "Return the file descriptor for the scanning device"
        return self.dev.fileno()

    def close(self):
        self.dev.close()

def open(devname):
    "Open a device for scanning"
    new=SaneDev(devname)
    return new
