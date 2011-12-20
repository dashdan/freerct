import Image
import xml.dom.minidom

fname = "freerct.xml"
tag = "direction"

class gameblock(object):
    def __init__(self,magic,version):
        self.images = []
        self.params = []
        self.magic = magic
        self.version = version

    def addimage(self,im):
        self.images.append(im)

    def addimages(self,im,width,height,first = [0],second = [0]):
        for i in first:
            for j in second:
                addimage(im.crop((first,second,first+width,second+height)))
                
    def addparam(self,typ,val):
        self.params.append((typ,val))
                
dom = xml.dom.minidom.parse(fname)

class rcdfile(object):
    def __init__(self,fdom):
        self.gameblocks = []
        self.fname = fdom.getAttribute("fname")
        self.magic = fdom.getAttribute("magic")
        self.version = fdom.getAttribute("version")
        gbdom = fdom.getElementsByTagName("gameblock")
        for i in gbdom:
            g = gameblock(i.getAttribute("magic"),i.getAttribute("version"))
            for node in i.childNodes:
                if node.nodeType == 1:
                    if node.nodeName == "addimages":
                        img = Image.open(node.getAttribute("fname"))
                        xorg = node.getAttribute("xorigin")
                        yorg = node.getAttribute("yorigin")
                        xstep = node.getAttribute("xstep")
                        ystep = node.getAttribute("ystep")
                        cols = node.getAttribute("colums")
                        rows = node.getAttribute("rows")
                        g.addimages(img,xstep,ystep,range(xorg,xorg+xstep*cols,xstep),range(yorg,yorg+ystep*rows,ystep))
                                                        # width,height moet er nog bij ipv xstep/ystep
                                                        # en ook de richting hor/vert
                    elif node.nodeName == "addimage":
                        g.addimage(Image.open(node.getAttribute("fname"))
                    elif node.nodeName == "attribute":
                        g.addparam(node.getAttribute("type"),node.getattribute("value"))
                    else:
                        print "unknown tag found",node.nodename
            self.gameblocks.append(g)

    def handleimages(fname,xorg,yorg,xstep,ystep,rows,cols,width = xstep,height = ystep):
                    
fdom = dom.getElementsByTagName("file")
for f in fdom:
    print f.getAttribute("id")
    if f.getAttribute("id") == tag:
        r = rcdfile(f)
        

