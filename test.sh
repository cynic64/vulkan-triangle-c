#!/bin/sh
gcc -g -o bin/runner $(pkg-config --cflags --libs vulkan) $(pkg-config --cflags --libs glfw3) $(pkg-config --cflags --libs cglm) $(pkg-config --cflags --libs check) src/*.c tests-src/*.c test-runner/runner.c 2>&1 | tee out-test.txt

test $? -eq 0 && bin/runner 2>&1
