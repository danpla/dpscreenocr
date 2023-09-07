This is a launcher to run an application bundled with its own
libraries, like AppImage or just binaries in an archive.


Why C instead of a shell script?
================================

The main reason why the launcher is written in C rather than being a
shell script with LD_LIBRARY_PATH trick is the need to choose between
the system and the bundled libstdc++ version. This requires using the
dlinfo() function with RTLD_DI_LINKMAP to get the path of the system
libstdc++ resolved by the linker.

Why not just use the bundled libstdc++ unconditionally? Although
libstdc++ itself is backward-compatible, the application can still
link with system libraries built with stock (newer) libstdc++, such as
graphics card drivers. If this happens, the linkage will fail as the
linker is forced to use an older libstdc++.


Usage
=====


Building
--------

To build the launcher, you need to set the following CMake variables:

  * LAUNCHER_EXE_PATH - path to the executable to be launched

  * LAUNCHER_LIB_DIR_PATH - path to the directory with bundled
    libraries

Both paths will be calculated relative to the launcher location, and
therefore should not be absolute.


Choosing between system and bundled libraries
---------------------------------------------

If you want the launcher to choose between the system and the bundled
library at runtime (you will usually need to do this for libstdc++),
the bundled library should be packaged especially:

  * Add a "fallback" directory in LAUNCHER_LIB_DIR_PATH. The word
    "fallback" in this context means "a fallback library to be used if
    the system one is older".

  * Add a directory inside "fallback" that has the same name as the
    name of the library to be passed to the linker. The launcher
    automatically collects all such subdirectories of "fallback".

  * The library directory inside "fallback" should have two files.

    The first is the actual library file, which, unlike the base
    library name (the name of the directory), should end with extra
    (usually two) version numbers. The launcher uses these numbers to
    decide which library is newer.

    The second is a relative symbolic link to the actual library file.
    It should have the same name as the directory. When the launcher
    processes the library directory, it will use this symbolic link to
    get the actual library path.

For example, if LAUNCHER_LIB_DIR_PATH is "lib" and the application
links against libstdc++.so.6, the actual library from you machine
(say, libstdc++.so.6.0.28) should be bundled like:

    lib/fallback/libstdc++.so.6/libstdc++.so.6
    lib/fallback/libstdc++.so.6/libstdc++.so.6.0.28

Where the first path is a symbolic link to libstdc++.so.6.0.28,
relative to the "lib/fallback/libstdc++.so.6" directory.


Debugging
---------

The launcher prints debugging information if the LAUNCHER_DEBUG
environment variable is set and is not equal to "0".
