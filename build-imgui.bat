cl.exe /c /W3 /nologo /I. /Zi /MDd /EHsc /Fdimgui.pdb /Fo:.\ *.cpp

copy imconfig.h imconfig.original.h
type imconfig.original.h ..\imgui-sfml\imconfig-SFML.h > imconfig.h