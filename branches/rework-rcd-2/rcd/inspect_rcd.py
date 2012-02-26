#!/usr/bin/env python
#
# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#
# Try to load an RCD file.
# Code is separate from the RCD creation by design.
#
import sys

class RCD(object):
    def __init__(self, fname):
        self.fname = fname
        fp = open(fname, 'rb')
        self.data = fp.read()
        fp.close()

    def get_size(self):
        return len(self.data)

    def name(self, offset):
        return self.data[offset:offset+4]

    def uint8(self, offset):
        return ord(self.data[offset])

    def uint16(self, offset):
        val = self.uint8(offset)
        return val | (self.uint8(offset + 1) << 8)

    def uint32(self, offset):
        val = self.uint16(offset)
        return val | (self.uint16(offset + 2) << 16)

blocks = {'8PAL' : 1, '8PXL' : 1, 'SPRT' : 2, 'SURF' : 3}

def list_blocks(rcd):
    stat = {}
    sz = rcd.get_size()

    rcd_name = rcd.name(0)
    rcd_version = rcd.uint32(4)
    print "%06X: (file header)" % 0
    print "    Name: " + repr(rcd_name)
    print "    Version: " + str(rcd_version)
    print
    if rcd_name != 'RCDF' or rcd_version != 1:
        print "ERROR"
        return

    offset = 8
    number = 1
    while offset < sz:
        name = rcd.name(offset)
        version = rcd.uint32(offset + 4)
        length = rcd.uint32(offset + 8)
        print "%06X (block %d)" % (offset, number)
        print "    Name: " + repr(name)
        print "    Version: " + str(version)
        print "    Length: " + str(length)
        if repr(name) in stat:
            stat[repr(name)] = stat[repr(name)] + 1
        else:
            stat[repr(name)] = 1
        #if name not in blocks or blocks[name] != version:
        #    print "ERROR"
        #    return

        if name == 'FUND' and version == 1:
            print "    FUND type: "   + str(rcd.uint16(offset + 12))
            print "    FUND width: "  + str(rcd.uint16(offset + 14))
            print "    FUND height: " + str(rcd.uint16(offset + 16))
            print "    FUND se_e: "   + str(rcd.uint16(offset + 18))
            print "    FUND se_s: "   + str(rcd.uint16(offset + 22))
            print "    FUND se_es: "  + str(rcd.uint16(offset + 26))
            print "    FUND ws_s: "   + str(rcd.uint16(offset + 30))
            print "    FUND ws_w: "   + str(rcd.uint16(offset + 34))
            print "    FUND ws_ws: "  + str(rcd.uint16(offset + 38))

        print

        assert length > 0
        offset = offset + 12 + length
        number = number + 1
    assert offset == sz
    print "Statistics"
    for i in stat:
        print i,stat[i]

if len(sys.argv) == 1:
    print "Missing RCD file argument."
    sys.exit(1)

for f in sys.argv[1:]:
    rcd = RCD(f)
    list_blocks(rcd)
