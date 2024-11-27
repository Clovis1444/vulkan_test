#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
cd "$SCRIPT_DIR" || exit 1

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
