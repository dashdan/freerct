import xml.dom.minidom

class Structures(object):
    def __init__(self):
        self.structs = {}
        self.magic = 'XXXX'
        self.version = 0

    def loadfromDOM(self,node):
        self.magic = node.getAttribute("magic")
        self.version = node.getAttribute("version")
        structnodes = node.getElementsByTagName("struct")
        for i in structnodes:
            struct = Struct()
            struct.loadfromDOM(i)
            # check for double magic/version ?
            self.structs[struct.magic] = struct

    def writeCPP(self):
        print "read from RCD with magic",self.magic,self.version
        ####if (!rcd_file.CheckFileHeader("RCDF", 1)) return "Could not read header";
        for magic in self.structs:
            self.structs[magic].writeCPPloadclass()

class Struct(object):
    def __init__(self):
        self.attributes = []
        self.cclass = ''
        self.magic = 'XXXX'
        self.version = 0

    def loadfromDOM(self,node):
        self.cclass = node.getAttribute("class")
        self.magic = node.getAttribute("magic")
        self.version = node.getAttribute("version")
        # calc a hash of attribute types ???
        anodes = node.getElementsByTagName("attribute")
        for i in anodes:
            attr = Attribute()
            attr.loadfromDOM(i)
            # check for double name ?
            self.attributes.append(attr)

    def writeCPPloadclass(self):
        print "/**"
        print " * %Sprite data for "+self.magic+" RCD structure."
        print " * @ingroup gui_sprites_group (why?)"
        print " */"
        print "struct "+self.cclass+" {"
        print "    void Clear();"
        print ""
        print "    bool IsLoaded() const;"
        for atr in self.attributes:
            atr.writeCPPstructdef()
        print "};"
        print ""
        print "/** Clear the",self.magic,"sprite data. */"
        print "void "+self.cclass+"::Clear()"
        print "{"
        for atr in self.attributes:
            atr.writeCPPcleartruct()
        print "}"
        print ""

        print "/**"
        print " * Load "+self.magic+" game block from a RCD file."
        print " * @param rcd_file RCD file used for loading."
        print " * @param length Length of the data part of the block."
        print " * @param sprites Map of already loaded sprites."
        print " * @return Loading was successful."
        print " */"
        print "bool "+self.cclass+"::Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)"
        print "{"
        total = 0
        for atr in self.attributes:
            total = total + atr.lengthof()
        print " if (length != ",total,") return false;"
        print ""
        for atr in self.attributes:
            atr.writeCPPget()
        print "}"
        print ""
        

class Attribute(object):
    def __init__(self):
        self.type = ''
        self.name = ''
        self.comment = ''
        self.count = 0
        self.const = ''

    def loadfromDOM(self,node):
        self.type = node.getAttribute("type")
        self.name = node.getAttribute("name")
        if self.type == "Sprite":
            self.count = node.getAttribute("count")
            if node.hasAttribute("const"):
                self.const = node.getAttribute("const")
        if node.firstChild != None:
            self.comment = node.firstChild.nodeValue
            #print self.comment,node.firstChild.nodeType
            # doe iets als er meer childeren are, which are of type 3 NODE_TEXT?

    def writeCPPstructdef(self):
        if self.type == "Sprite":
            print "    Sprite *"+self.name+"["+self.count+"]; ///< "+self.comment
        else:
            print "    "+self.type+" "+self.name+"; ///< "+self.comment
    def writeCPPcleartruct(self):
        if self.type == "Sprite":
            print "    for (uint sprnum = 0; sprnum <",self.count,"; sprnum++) {"
            print "        this->"+self.name+"[sprnum] = NULL;"
            print "    }"
        else:
            print "    this->"+self.name+" = 0;"

    def writeCPPget(self):
        gettype = ''
        if self.type == 'uint8':
            gettype = "GetUInt8"
        elif self.type == 'int8':
            gettype = "GetInt8"
        elif self.type == 'uint16':
            gettype = "GetUInt16"
        elif self.type == 'int16':
            gettype = "GetInt16"
        elif self.type == 'uint32':
            gettype = "GetUInt32"
        elif self.type == 'int32':
            gettype = "GetInt32"
        elif self.type == "Sprite":
            if self.const != '':
                print "#assert",self.const,"==",self.count
            print "for (uint sprnum = 0; sprnum <",self.count,"; sprnum++) {"
            print "  uint32 val = rcd_file->GetUInt32();"
            print "  Sprite *spr;"
            print "  if (val == 0) {"
            print "    spr = NULL;"
            print "  } else {"
            print "    SpriteMap::const_iterator iter = sprites.find(val);"
            print "    if (iter == sprites.end()) return false;"
            print "    spr = (*iter).second;"
            print "  }"
            print "  this->"+self.name+"[sprnum] = spr;"
            print "}"
        else:
            print "ERROR Unknown type",self.type
        if gettype != '':
            print "this->"+self.name+" = rcd_file->"+gettype+"();"
        
    def lengthof(self):
        # return the size in C structure length of this attribute
        if self.type == 'uint8' or self.type == 'int8':
            return 1
        if self.type == 'uint16' or self.type == 'int16':
            return 2
        if self.type == 'uint32' or self.type == 'int32':
            return 4
        if self.type == 'Sprite':
            return int(self.count) * 4

dom = xml.dom.minidom.parse("rcdstructure.xml")
rcdstructure = Structures()
rcdstructure.loadfromDOM(dom.getElementsByTagName("structures").item(0))

rcdstructure.writeCPP()
