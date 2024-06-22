This is a launcher to run an application bundled with its own
libraries.


Why C instead of a shell script?
================================

The main reason why the launcher is written in C rather than being a
shell script with LD_LIBRARY_PATH trick is the need to choose between
the system and the bundled libstdc++ version. This requires using the
dlinfo() function with RTLD_DI_LINKMAP to get the path of the system
libstdc++ resolved by the linker.

Why not just use the bundled libstdc++ unconditionally? Although
libstdc++ itself is backward-compatible, the application can still
link against system libraries built with stock (newer) libstdc++. If
this happens, the linkage will fail as the linker is forced to use the
bundled (older) libstdc++, which lacks the necessary symbols.


Usage
=====


Configuration file
------------------

The launcher requires a configuration file, which should be in the
same directory as the launcher and have the same name as the launcher
binary with an extra ".cfg" extension. Each line in this file contains
a key-value pair, separated by one or more spaces. The following are
required:

  * exe - a path to the executable to be launched
  * lib_dir - a path to the directory with bundled libraries

Both paths will be calculated relative to the launcher location, and
therefore should not be absolute.

A CFG file for a launcher located in the installation prefix might
look like this:

    exe     bin/your-app
    lib_dir lib

A CFG file for an AppImage (AppRun.cfg) will be:

    exe     usr/bin/your-app
    lib_dir usr/lib


Choosing between system and bundled libraries
---------------------------------------------

If you want the launcher to choose between the system and the bundled
library at runtime (you will usually need to do this for libstdc++),
the bundled library should be packaged especially:

  * Add a "fallback" directory in the "lib_dir" path from CFG file.
    The word "fallback" in this context means "a library to be used if
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

For example, if the "lib_dir" path in the CFG file is "lib" and the
application links against libstdc++.so.6, the actual library from you
machine (say, libstdc++.so.6.0.28) should be bundled like:

    lib/fallback/libstdc++.so.6/libstdc++.so.6
    lib/fallback/libstdc++.so.6/libstdc++.so.6.0.28

Where the first path is a symbolic link to libstdc++.so.6.0.28,
relative to the "lib/fallback/libstdc++.so.6" directory.


LAUNCHER_ARGV0 environment variable
-----------------------------------

Your executable can use the LAUNCHER_ARGV0 environment variable in
cases where you need the launcher's argv[0] instead of the one passed
to the launched executable.

If LAUNCHER_ARGV0 is already set, the launcher will leave it
unchanged. If the ARGV0 environment variable is set, its value is used
for LAUNCHER_ARGV0 (this is for AppImage support). Otherwise,
LAUNCHER_ARGV0 is set to the launcher's actual argv[0].


Debugging
---------

The launcher prints debugging information if the LAUNCHER_DEBUG
environment variable is set and is not equal to "0".
