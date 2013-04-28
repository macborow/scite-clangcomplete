scite-clangcomplete
===================

libclang based C/C++ code autocompletion plugin for SciTE

Introduction
============

SciTE is a brilliant, lightweight editor which I use a lot for writing code in multiple programming languages. However, one feature
that is missing is context based code completion - the only option available by default is ctags (which could be good enough
for small projects, but I always wanted something like "intellisense" which produces a smart list of code completion suggestions based
on included header files and that can auto complete struct/class members, knows about namespaces, etc.)

This library is a personal project to fill that gap by using libclang for autocompletion - it is rather basic at the moment but does the job.
One of the goals was to create a stand alone plugin instead of modifying the source of SciTE directly, so I was trying to use whatever features
SciTE provides to display context menus, etc. The plugin consists of two parts: DLL written in C++ and a SciTE startup script written in LUA.

There is definitely space for improvement and I have other ideas that I might implement in the future (not to mention other functionality of
clang that can be integrated in the editor, e.g. code navigation). Do not hesitate to take the code and modify it according to your needs.

Prerequisites and building
==========================

The process is rather complicated and I will describe it in more detail at a later time. For now, here is the minimum you need to know.
I used MinGW gcc 4.7.2 toolchain on Windows and GNU Make 3.81 - you need to build all prerequisites with the same compiler that you intend to use for
the scite-clangcomplete plugin.

Before building, you will need to create a directory and download the following (use the command below to ensure
the names of directories match paths in the makefile):
  1. [Scintilla](http://sourceforge.net/projects/scintilla/) source code - I suggest getting latest sources from Mercurial repo
  
```bash
hg clone http://hg.code.sf.net/p/scintilla/code scintilla
```

  2. SciTE source code:

```bash
hg clone http://hg.code.sf.net/p/scintilla/scite scite
```

  3. [LLVM](http://clang.llvm.org/get_started.html#build) source (or libclang built with your compiler)

```bash
svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
```

  4. [clang](http://clang.llvm.org/get_started.html#build) source (or libclang built with your compiler)

```bash
cd llvm/tools
svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
cd ../..
```

Build the following libraries and SciTE (Sc1.exe) with the compiler of your choice (I will include detailed instructions later,
for now refer to the information on Scintilla/SciTE and LLVM/clang web pages).

After that, you need to create libscite.a that need to be linked with the project:

```bash
cd scite/
pexports.exe scite.exe > scite.def
dlltool.exe -d scite.def -l libscite.a
```

At that point you should be ready to build clangcomplete.dll:

```bash
cd scite-clangcomplete
make
```

Copy scite/bin/Sc1.exe, SciTEGlobal.properties, SciTEStartup.lua, clangcomplete.dll to target directory (TODO: add "installation" 
step to the makefile). Run Sc1 and create a test .cpp file - auto complete will be activated when Ctrl+Space is pressed
(or Alt+Space, if you want do not want to use include paths defined in .properties file). Not that you might need to type in some code
before clang will give any autocompletion suggestions. You might also want to set the font to monospace.

License
=======

zlib license

Copyright (c) 2013, Maciej Borowik

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.



