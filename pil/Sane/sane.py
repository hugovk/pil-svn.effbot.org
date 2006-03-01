
import Image
import _sane

from _sane import *

class Option:
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

class SaneDev:
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

    def get_parameters(self): return self.dev.get_parameters()
    def get_options(self): return self.dev.get_options()
    def start(self): return self.dev.start()
    def cancel(self): return self.dev.cancel()
    def fileno(self): return self.dev.fileno()
    def snap(self):
	format, last_frame, (xsize, ysize), depth, bytes_per_line = self.get_parameters()
	im=Image.new(format, (xsize,ysize))
	self.dev.snap( im.im.id )
	return im

def open(devname):
    new=SaneDev(devname)
    return new
