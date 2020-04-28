#!/bin/sh
gcc -g -o demo/triangle $(pkg-config --cflags --libs vulkan) $(pkg-config --cflags --libs glfw3) src/* demo/triangle.c

test $? -eq 0 && demo/triangle
