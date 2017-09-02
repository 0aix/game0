# Game0 - Stacking Game

Stacking game based on Adriel's design [http://graphics.cs.cmu.edu/courses/15-466-f17/game0-designs/aluo/](http://graphics.cs.cmu.edu/courses/15-466-f17/game0-designs/aluo/)

Left-click to place a block. Each block must be placed on the previous, and the speed of the next block will increase. Any part of a block that's not resting on the previous block will be cut off. When you reach the top of the screen, you win!

When you win, the game can be started over, but the base speed of the blocks will increase.


![](https://github.com/0aix/game0/blob/master/screenshot.png?raw=true)

## Base0

Base0 is the starter code for the game0 in the 15-466-f17 course. It was developed by Jim McCann.

## Requirements

 - glm
 - libSDL2
 - modern C++ compiler

On Linux or OSX these requirements should be available from your package manager without too much hassle.

## Building

### Linux
```
  g++ -g -Wall -Werror -o main main.cpp Draw.cpp `sdl2-config --cflags --libs` -lGL
```
or:
```
  make
```

### OSX
```
  clang++ -g -Wall -Werror -o main main.cpp Draw.cpp `sdl2-config --cflags --libs`
```
or:
```
  make
```

### Windows

Before building, clone [kit-libs-win](https://github.com/ixchow/kit-libs-win) into the `kit-libs-win` subdirectory:
```
  git clone https://github.com/ixchow/kit-libs-win
```
Now you can:
```
  nmake -f Makefile.win
```
or:
```
  cl.exe /EHsc /W3 /WX /MD /Ikit-libs-win\out\include /Ikit-libs-win\out\include\SDL2 main.cpp Draw.cpp gl_shims.cpp /link /SUBSYSTEM:CONSOLE /LIBPATH:kit-libs-win\out\lib SDL2main.lib SDL2.lib OpenGL32.lib
  copy kit-libs-win\out\dist\SDL2.dll .
```

