# quicklinks-taskbar-cpp
Create Quick Links/Shortcuts in Taskbar for windows 11, you can define your links/shortcut in config.json, cpp coding with cppwinrt/wil/IUIAutomation/jumplist

## Compile 
open directory & cmake configure & cmake build, using vs code with cmake plugin and VS Build Tools 2022 installed.

you should use cmake of MS VS Build Tools 2022, it is normally at 

```cmd
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
```

Compile as following

```cmd
cmake -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -S. -Bbuilds/vs-multi-vcpkg -G "Ninja Multi-Config"

cmake.exe --build builds/vs-multi-vcpkg --config Debug --target quicklinks
```

## Usage

- Run Quicklinks.exe (with config.json in same directory)
- Pin Quicklinks.exe in taskbar
- Right/or left click the icon in taskbar, or WIN+\<number>

## Screen 

![screen](./docs/image.png)

## License

- GPLv3, Copyright by corbamico@163.com  
- library nlohmann/json, and library microsoft/wil under LICENSE.MIT
