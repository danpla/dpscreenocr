#!/bin/sh
#
# This is a simple helper script to create a bundle with a single
# "docker run [...] build-bundle" call, without having to go into a
# shell inside the container and mess around with CMake.

set -eu

printHelp()
(
cat << EOF
Usage: $0 SOURCE_CODE_DIR

Build a bundle from the program source code in SOURCE_CODE_DIR. The
TAR.XZ archive will be written to the current working directory.
EOF
)

if [ "$#" -ne 1 ] || [ -z "$1" ]; then
    printHelp
    exit 1
fi

SOURCE_CODE_DIR="$1"

# Note we are using "/tmp" that is inside the container.
BUILD_DIR="/tmp/$(basename $0)-build-dir"

cmake \
    -S "$SOURCE_CODE_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DDPSO_USE_DEFAULT_TESSERACT_DATA_PATH=No \
    -DDPSO_DYNAMIC_CURL=Yes

# There's no need for --parallel here, because the "bundle_archive"
# target is already built in the parallel mode under the hood.
cmake --build "$BUILD_DIR" --target bundle_archive

mv --force "$BUILD_DIR"/*".tar.xz" .
