
# Get the path set up properly
import sys ; sys.path.append('.')

import sane
print 'SANE version:', sane.init()
print 'Available devices=', sane.get_devices()

scanner=sane.open('qcam:0x378')
print 'SaneDev object=', scanner
print 'Device parameters:', scanner.get_parameters()

# Set scan parameters
scanner.contrast=170 ; scanner.brightness=150 ; scanner.white_level=190
scanner.depth=6
scanner.br_x=320 ; scanner.br_y=240

# Initiate the scan
scanner.start()

# Get an Image object 
# (For my B&W QuickCam, this is a grey-scale image.  Other scanning devices
#  may return a 
im=scanner.snap()

# Write the image out as a GIF file
im.save('foo.gif')

# The show method() simply saves the image to a temporary file and calls "xv".
#im.show()
