import Image
import xml.dom.minidom

# invoer voor dit commando, het moet eigenlijk via de command line, maar voor te testen is dit handiger
fname = "freerct.xml"
tag = "foundation"

class gameblock(object):
    def __init__(self,magic,version):
        self.images = []
        self.params = []
        self.magic = magic
        self.version = version

    def addimage(self,im):
        """
        Add an sprite image to the gameblock collection
        """
        self.images.append(im)

    def addimages(self,im,width,height,first,second):
        """
        Add multiple sprites from one image.
        On every permutation of a first and second coordinates
        There will be len(first)*len(second) sprites added
        """
        #print "addimages",width,height,first,second
        for i in first:
            for j in second:
                self.addimage(im.crop((i,j,i+width,j+height)))
                
    def addparam(self,typ,val):
        """
        Add a parameter to the gameblock
        This could represent the zoom-level, or type of width, used in conjunction with this collection of sprites
        typ can be: uint8 int8 uint16 int16 int32
        """
        self.params.append((typ,val))

    def LoadFromDOM(self,domnode):
        """
        Fill the gameblock from a DOM object
        """
        for node in domnode:
            if node.nodeType == node.ELEMENT_NODE:
                if node.nodeName == "addimages":
                    img = Image.open(node.getAttribute("fname"))
                    xorg = int(node.getAttribute("xorigin"))
                    yorg = int(node.getAttribute("yorigin"))
                    xstep = int(node.getAttribute("xstep"))
                    ystep = int(node.getAttribute("ystep"))
                    cols = int(node.getAttribute("colums"))
                    rows = int(node.getAttribute("rows"))
                    self.addimages(img,xstep,ystep,range(xorg,xorg+xstep*cols,xstep),range(yorg,yorg+ystep*rows,ystep))
                                                    # width,height moet er nog bij ipv xstep/ystep
                                                    # en ook de richting hor/vert
                elif node.nodeName == "addimage":
                    self.addimage(Image.open(node.getAttribute("fname")))
                elif node.nodeName == "attribute":
                    self.addparam(node.getAttribute("type"),node.getAttribute("value"))
                else:
                    print "unknown tag found",node.nodename,"ignored"
                    
    def SaveToFile(self):
        print "write header",self.magic,self.version
        print "size...."
        for i in self.params:
            print i[0],i[1]
        for i in self.images:
            print "write sprite"
        
dom = xml.dom.minidom.parse(fname)

class rcdfile(object):
    def __init__(self,fdom):
        self.gameblocks = []
        self.fname = fdom.getAttribute("fname")
        self.magic = fdom.getAttribute("magic")
        self.version = fdom.getAttribute("version")
                
    def LoadFromDOM(self,fdom):
        gbdom = fdom.getElementsByTagName("gameblock")
        for i in gbdom:
            g = gameblock(i.getAttribute("magic"),i.getAttribute("version"))
            g.LoadFromDOM(i.childNodes)
            self.gameblocks.append(g)
            
    def SaveToFile(self):
        print "open",self.fname,"for write"
        print "write header magic + version"
        for i in self.gameblocks:
            i.SaveToFile()
                    
fdom = dom.getElementsByTagName("file")
for f in fdom:
    #print f.getAttribute("id")
    if f.getAttribute("id") == tag:
        r = rcdfile(f)
        r.LoadFromDOM(f)
        r.SaveToFile()
