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
        # calc a hash of attributes ???
        anodes = node.getElementsByTagName("attribute")
        for i in anodes:
            attr = Attribute()
            attr.loadfromDOM(i)
            # check for double name ?
            self.attributes.append(attr)
            
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
        if node.hasChildsNodes():
            print node.Child
            # doe iets

dom = xml.dom.minidom.parse('rcdstructure.xml')
rcdstructure = Structures()
rcdstructure.loadfromDOM(dom)

