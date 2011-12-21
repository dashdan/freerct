
import Image
import imageobjectclass
import xml.dom.minidom
from rcdlib import output

# to do:
# use xml to create cstrutures and documentation?

# invoer voor dit commando, het moet eigenlijk via de command line, maar voor te testen is dit handiger
# tag parameter zou ook filenaam kunnen zijn, dan kan de tag weg
# als de tag/filenaam all is, dan alle bestanden maken
fname = "freerct.xml"
tag = "foundation"

class gameblock(object):
    def __init__(self,magic,version):
        self.images = []
        self.params = []
        self.magic = magic
        self.version = version

    def addimage(self,im,xoffset, yoffset):
        """
        Add an sprite image to the gameblock collection
        """
        x = imageobjectclass.ImageObject(im,xoffset, yoffset)
        self.images.append(x)

    def addimages(self,im,width,height,first,second,xoffset, yoffset):
        """
        Add multiple sprites from one image.
        On every permutation of a first and second coordinates
        There will be len(first)*len(second) sprites added
        """
        #print "addimages",width,height,first,second
        for i in first:
            for j in second:
                self.addimage(im.crop((i,j,i+width,j+height)).load(),xoffset, yoffset)
                
    def addparam(self,typ,val):
        """
        Add a parameter to the gameblock
        This could represent the zoom-level, or type of width, used in conjunction with this collection of sprites
        typ can be: uint8 int8 uint16 int16 int32
        """
        self.params.append((typ,int(val)))

    def sizeofparams(self):
        """
        Calculate the size of the total paramters of the game block
        """
        sizes = {'int8':1, 'uint8':1, 'int16':2, 'uint16':2, 'uint32':4}
        total = 0
        for i in self.params:
            total = total + sizes[i[0]]
        return total
    
    def writeparams(self, out):
        #Block.write(self, out)
        for fldtype,fldval in self.params:
            if fldtype == 'int8':
                out.int8(fldval)
            elif fldtype == 'uint8':
                out.uint8(fldval)
            elif fldtype == 'int16':
                out.int16(fldval)
            elif fldtype == 'uint16':
                out.uint16(fldval)
            elif fldtype == 'uint32':
                out.uint32(fldval)
            else:
                raise ValueError("Unknown field-type: %r" % fldtype)

    def writesprite(self,out,im):
        """
        Write out one sprites to file

        return file location
        """
        # handel iets met lege sprites
        pxl8pos = out.tell()
#        print "write sprite:out.image(iets)"
        im.write_8PXL(out)  # trans en skip_crop

#        out.store_magic('8PXL')
#        out.uint32(1)       # version
#        out.uint32(4+im.height*4)       # size of structure
#        out.uint16(im.width)
#        out.uint16(im.height)
        # write im.height() pointers
        
        
        sprtpos = out.tell()
        out.store_magic('SPRT')
        out.uint32(2)       # version
        out.uint32(8)       # size of structure
        # waarom is de sprite niet onderdeel van SPRT ?
        out.int16(0)       #x offset !!!!!!!!!moet nog iets gebeuren
        out.int16(0)       #y offset !!!!!!!!!!!idem
        out.uint32(pxl8pos) # the real sprite location
        return sprtpos

    def LoadFromDOM(self,domnode):
        """
        Fill the gameblock from a DOM object
        """
        for node in domnode:
            if node.nodeType == node.ELEMENT_NODE:
                if node.nodeName == "addimages":
                    img = Image.open(node.getAttribute("fname"))    # geef de fnaam door ipv image object
                    xorg = int(node.getAttribute("xorigin"))   # functie voor controle attribute
                    yorg = int(node.getAttribute("yorigin"))
                    xstep = int(node.getAttribute("xstep"))
                    ystep = int(node.getAttribute("ystep"))
                    cols = int(node.getAttribute("colums"))
                    rows = int(node.getAttribute("rows"))
                    
                    xoffset = int(node.getAttribute("xoffset"))    #-32 
                    yoffset = int(node.getAttribute("yoffset"))     #-33
                    width = int(node.getAttribute("width"))
                    height = int(node.getAttribute("height"))
                    self.addimages(img,width,height,range(yorg,yorg+ystep*rows,ystep),range(xorg,xorg+xstep*cols,xstep),xoffset, yoffset)
                                                    # width,height moet er nog bij ipv xstep/ystep
                                                    # en ook de richting hor/vert
                                                    # als cols = 1, dan is xstep niet nodig
                                                    # als rows = 1, dan is ystep not needed
                elif node.nodeName == "addimage":
                    xoffset = int(node.getAttribute("xoffset"))
                    yoffset = int(node.getAttribute("yoffset"))
                    width = int(node.getAttribute("xpos"))    #optional
                    height = int(node.getAttribute("ypos"))    #optional
                    self.addimage(Image.open(node.getAttribute("fname")),xoffset, yoffset)
                    # add some support for empty image
                elif node.nodeName == "attribute":
                    self.addparam(node.getAttribute("type"),node.getAttribute("value"))
                else:
                    print "unknown tag found",node.nodename,"ignored"
                    
    def writetofile(self,out):
        """
        Write gameblock to file
        First write sprites, add file location as parameter
        """
        for i in self.images:
            self.addparam("uint32",self.writesprite(out,i))
        out.store_magic(self.magic)
        out.uint32(self.version)
        out.uint32(self.sizeofparams())
        self.writeparams(out)
        
dom = xml.dom.minidom.parse(fname)

class rcdfile(object):
    def __init__(self,fdom):
        self.gameblocks = []
        self.fname = fdom.getAttribute("fname")
        self.magic = fdom.getAttribute("magic")
        self.version = int(fdom.getAttribute("version"))
                
    def LoadFromDOM(self,fdom):
        gbdom = fdom.getElementsByTagName("gameblock")
        for i in gbdom:
            g = gameblock(i.getAttribute("magic"),int(i.getAttribute("version")))
            g.LoadFromDOM(i.childNodes)
            self.gameblocks.append(g)
            
    def writetofile(self):
        out = output.FileOutput()
        out.set_file(self.fname)
        out.store_magic(self.magic)
        out.uint32(self.version)
        for i in self.gameblocks:
            i.writetofile(out)
        out.close()
                    
fdom = dom.getElementsByTagName("file")
for f in fdom:
    #print f.getAttribute("id")
    if f.getAttribute("id") == tag:
        r = rcdfile(f)
        r.LoadFromDOM(f)
        r.writetofile()
