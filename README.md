# s2cpp-monorepo

This repository contains the source code for all the implementations and demos used in the tutorials on [https://severisuominen.github.io](https://severisuominen.github.io)  

## How to build

Project can be build with <strong>CMake 3.8 (or newer)</strong>. All depedencies are included or fetched automatically:

### Generate project files

Compiler version supporting <strong>C++17</strong> required. 

Clang, GCC and MSVC should all compile just fine, but only tested with Clang and MSVC (on Windows).

To generate project files:

```shell
# building out of source '-B target'
cmake -S . -B build
```
or if you want to use <strong>Ninja and Clangd</strong> setup (non Visual Studio project setup, requires Clang):

```shell
cmake -G "Ninja Multi-Config" -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
# personal preference, copying compile_command.json to source root
cp build/compile_commands.json .
```

### Build binaries

```shell
cmake --build build --config Debug
```


