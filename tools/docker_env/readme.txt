This directory contains files to create a Docker image, which is
primarily intended to be used as an AppImage build environment. The
image contains everything needed for the task, from all the necessary
libraries to the linuxdeploy tool.

To build the image, use:

  docker build --force-rm -t dpscreenocr-env .

To build an AppImage, run:

  docker run --rm -v "$PWD:/workspace" dpscreenocr-env \
    build-appimage SOURCE_CODE_DIR

Where SOURCE_CODE_DIR is a path to the dpScreenOCR source code
directory, relative to the current working directory.

If you need full control over the build process, use:

  docker run --rm -it -v "$PWD:/workspace" dpscreenocr-env

The above command will drop you into the bash shell, with the current
working directory mounted as the "/workspace" directory inside the
container. To build the AppImage, follow the instructions in the
"doc/building-unix.txt" file in dpScreenOCR source code directory.
When finished, type "exit" to exit the container.
