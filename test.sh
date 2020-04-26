#!/bin/sh
gcc -g -o test-runner/runner $(pkg-config --cflags --libs vulkan) $(pkg-config --cflags --libs glfw3) $(pkg-config --cflags --libs check) src/* tests-src/* test-runner/runner.c

test $? -eq 0 && test-runner/runner
