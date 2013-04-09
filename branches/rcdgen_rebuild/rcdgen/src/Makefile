# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: clean

INCLUDES=
LIBS=-lpng
CXXFLAGS=-Wall -g
CXX=g++
DEPEND=deps
TARGET=rcdgen
CXXFILES=image.cpp main.cpp
OBJFILES=$(CXXFILES:.cpp=.o)

$(TARGET): $(DEPEND) $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJFILES) $(LIBS)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $(INCLUDES) $<

$(DEPEND): $(CXXFILES)
	for f in $(CXXFILES);\
	do $(CXX) -MM $(CXXFLAGS) $(INCLUDES) $$f;\
	done > $(DEPEND)

clean:
	for i in $(OBJFILES) $(DEPEND) $(TARGET);\
	do if test -f $$i; then rm $$i; fi;\
	done



ifndef NODEPS
include $(DEPEND)
endif
