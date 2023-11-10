This directory contains files to create a Docker image intended to be
used as a build environment for a bundle or AppImage. The image
contains all the libraries and tools needed for the task.

To build the image, use:

  docker build --force-rm -t dpscreenocr-build-env .

To build a TAR.XZ bundle archive, run:

  docker run --rm -v "$PWD:/workspace" dpscreenocr-build-env \
    build-bundle SOURCE_CODE_DIR

Where SOURCE_CODE_DIR is a path to the dpScreenOCR source code
directory, relative to the current working directory.

If you need full control over the build process, use:

  docker run --rm -it -v "$PWD:/workspace" dpscreenocr-build-env

The above command will drop you into the bash shell, with the current
working directory mounted as the "/workspace" directory inside the
container. Follow the instructions in the "doc/building-unix.txt" file
in the dpScreenOCR source code directory, then type "exit" to exit the
container when you are done.
