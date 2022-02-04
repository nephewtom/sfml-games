# 16 games using SFML

Variations of games in:

https://www.youtube.com/user/FamTrinli/videos

## Compile 00 box2d on Windows

Make cl.exe and cmake.exe available on path
```
cd box2d
.\build.bat 
cd ..

cp build-imgui-sfml.bat imgui-sfml\build.bat
cp build-imgui.bat imgui\build.bat

cd imgui
.\build.bat

cd ..\imgui-sfml
.\build.bat

cd "..\00 box2d"
.\build.bat
```