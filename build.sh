#! /bin/bash

gcc -std=gnu99 -lGL -lGLU -lglut -lm -g -o wave wave.c || exit 1
gcc -std=gnu99 -lGL -lGLU -lglut -lm -g -o wave3d wave3d.c || exit 1

