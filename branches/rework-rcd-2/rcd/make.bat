set python=\python27\python.exe

rem borderhighlight_8bpp64.rcd: borderhighlight_8bpp64.py ../sprites_src/terrain/groundtiles/borderhighlight8bpp64_masked.png
%python% borderhighlight_8bpp64.py --output borderhighlight_8bpp64.rcd ../sprites_src/terrain/groundtiles/borderhighlight8bpp64_masked.png

rem foundation_8bpp64.rcd: foundation_8bpp64.py ../sprites_src/terrain/foundation/template8bpp64_masked.png
%python% foundation_8bpp64.py --output foundation_8bpp64.rcd ../sprites_src/terrain/foundation/template8bpp64_masked.png

rem groundtile_8bpp64.rcd: groundtile_8bpp64.py ../sprites_src/terrain/groundtiles/template8bpp64_masked.png ../sprites_src/terrain/groundtiles/tileselection8bpp64_masked.png
%python% groundtile_8bpp64.py --output groundtile_8bpp64.rcd --test ../sprites_src/terrain/groundtiles/tileselection8bpp64_masked.png ../sprites_src/terrain/groundtiles/template8bpp64_masked.png

rem cursor_groundtile_8bpp64.rcd: cursor_groundtile_8bpp64.py ../sprites_src/terrain/groundtiles/tileselection8bpp64_masked.png
%python% cursor_groundtile_8bpp64.py --output cursor_groundtile_8bpp64.rcd ../sprites_src/terrain/groundtiles/tileselection8bpp64_masked.png

rem cornerhighlight_8bpp64.rcd: cornerhighlight_8bpp64.py ../sprites_src/terrain/groundtiles/cornerhighlight8bpp64_masked.png
%python% cornerhighlight_8bpp64.py --output cornerhighlight_8bpp64.rcd ../sprites_src/terrain/groundtiles/cornerhighlight8bpp64_masked.png

rem path_8bpp64.rcd: path_8bpp64.py ../sprites_src/paths/path/template8bpp64.png
%python% path_8bpp64.py --output path_8bpp64.rcd ../sprites_src/paths/path/template8bpp64.png

rem buildarrows_8bpp64.rcd: buildarrows_8bpp64.py ../sprites_src/objects/1x1/gui/orthbuildmark8bpp64.png
%python% buildarrows_8bpp64.py --output buildarrows_8bpp64.rcd ../sprites_src/objects/1x1/gui/orthbuildmark8bpp64.png

rem gui.rcd: guis.py ../sprites_src/gui/GUIEles.png
%python% guis.py

