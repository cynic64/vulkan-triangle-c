#!/bin/sh
gcc -g -o bin/triangle $(pkg-config --cflags --libs vulkan) $(pkg-config --cflags --libs glfw3) $(pkg-config --cflags --libs cglm) src/* demo/triangle.c 2>&1 | tee out-run.txt

test $? -eq 0 && bin/triangle
