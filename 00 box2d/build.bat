cl.exe /I..\sfml\include /I..\box2d\include /I.\ box2d.cpp /MDd /link /libpath:..\sfml\lib sfml-system-d.lib sfml-window-d.lib sfml-graphics-d.lib sfml-audio-d.lib /libpath:..\box2d\build\bin\Debug box2d.lib /out:box2d.exe
