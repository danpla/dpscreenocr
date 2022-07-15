
[dpScreenOCR website]: https://danpla.github.io/dpscreenocr
[Language packs]: https://danpla.github.io/dpscreenocr/languages


# About dpScreenOCR

dpScreenOCR is a free and open-source program to recognize text on the
screen. Powered by [Tesseract][], it supports more than 100 languages
and can split independent text blocks, e.g. columns.

[Tesseract]: https://en.wikipedia.org/wiki/Tesseract_(software)


# Installation


## Installing dpScreenOCR


### Unix-like systems

The [dpScreenOCR website][] provides several download options,
including repositories for Debian, Ubuntu, and derivatives. If you
don't find a suitable choice for your system, download the source code
tarball, unpack it, and follow the instructions in the
"doc/building-unix.txt" file.


### Windows

The [dpScreenOCR website][] provides an installer and a ZIP archive.
The latter doesn't need installation: unpack it anywhere and run
dpscreenocr.exe.


## Installing languages


### Unix-like systems

Use your package manager to install languages for Tesseract. The
package names may vary across systems, but they usually start with
"tesseract" and end with a language code or name. For example, the
package for German have the following names:

*   "tesseract-ocr-deu" on Debian, Ubuntu, and derivatives
*   "tesseract-data-deu" on Arch Linux
*   "tesseract-langpack-deu" on Fedora
*   "tesseract-ocr-traineddata-german" on openSUSE

When searching for a language, be aware that some codes are not from
[ISO 639-3][]. In particular, "frk" is German Fraktur rather than
Frankish. The Tesseract developers are aware of this and will
probably fix the code in the future (see issues
[68][tessdata-frk-issue-68], [49][tessdata-frk-issue-49], and
[61][tessdata-frk-issue-61]); meanwhile, if "frk" is described as
"Frankish" in your package manager, you can report the problem to the
package maintainer.

There are also two special packs that provide extra features rather
than languages: "osd" (automatic script and orientation detection) and
"equ" (math and equation detection). dpScreenOCR doesn't use them.

[ISO 639-3]: https://en.wikipedia.org/wiki/ISO_639-3
[tessdata-frk-issue-68]: https://github.com/tesseract-ocr/tessdata_best/issues/68
[tessdata-frk-issue-49]: https://github.com/tesseract-ocr/tessdata/issues/49
[tessdata-frk-issue-61]: https://github.com/tesseract-ocr/langdata/issues/61


### Windows

dpScreenOCR for Windows is shipped with the English language pack. To
install other languages, visit the [Languages][Language packs] page,
download ".traineddata" files you want, and place them in the
`C:\Users\(your name)\AppData\Local\dpscreenocr\tesseract5_data`
folder. To quickly navigate to this folder, press Windows + R to open
"Run" and paste `%LOCALAPPDATA%\dpscreenocr\tesseract5_data`. You can
also paste this path to the folder address bar of File Explorer.


# Usage


dpScreenOCR is simple to use:

1.  Choose languages in the [Main tab].
2.  Move the mouse pointer near the screen area containing text and
    press the hotkey shown in the [Main tab] to start the selection.
3.  Move the mouse so that the selection covers the text and press the
    hotkey again.

After these steps, dpScreenOCR will recognize the text from the
selected area and process it according to the actions from the
[Actions tab].


## Main tab


### Status

Status describes the current state of dpScreenOCR. Green means the
program is ready to use, and you can press the [Hotkey] to start
selection. Yellow shows the progress of recognition. Red warns that
the program needs some setup, and you will not be able to start
selection until the problem is fixed.


### Character recognition


#### Split text blocks

If this option is enabled, dpScreenOCR will try to detect and split
independent text blocks, e.g. columns. This behavior is best described
by the following picture, which shows a two-column text layout (A)
recognized with (B) and without (C) the "Split text blocks" option:

![](manual-data/split.svg)


#### Languages

The language list shows available language packs that dpScreenOCR can
use to recognize text. You can choose more than one language, but be
aware that this may slow down recognition and reduce its accuracy.

Read the "[Installing languages]" section on how to install language
packs.


### Hotkey

The hotkey starts and ends the on-screen selection. To cancel the
selection, press Escape.

The hotkey is global: it works even if dpScreenOCR's window is
minimized. If pressing the hotkey has no effect, it probably means
that another program is already using it. In this case, try another
key combination.


## Actions tab

The Actions tab lets you choose what to do with the recognized text:
copy to the clipboard, add to history (see the [History tab]), or pass
as an argument to an executable.


### Run executable

The "Run executable" action will run an executable with the recognized
text as the first argument. The entry expects either an absolute path
to the executable, or just its name in case it's located in one of the
paths of your PATH environment variable.


#### Running scripts on Unix-like systems

Before using your script, make sure it starts with a proper
[shebang][] and you have the execute permission (run
`chmod u+x your_script`).

Here is an example Unix shell script that translates the recognized
text to your native language using [Translate Shell][] and displays
the translation as a desktop notification.

    #!/bin/sh

    notify-send "Translation" $(trans -b "$1")

[Shebang]: https://en.wikipedia.org/wiki/Shebang_(Unix)
[Translate Shell]: https://www.soimort.org/translate-shell/


#### Running scripts on Windows


##### Batch files

dpScreenOCR doesn't run batch files (".bat" or ".cmd") because there's
no way to safely pass them arbitrary text. Please use Python or any
other scripting language instead.


##### Creating file associations

Before using a script, make sure that the file association is
configured correctly so that you can launch the script just by its
file name, without mentioning the interpreter explicitly. The simplest
way to test this is to type the name of the script with some arguments
in cmd.exe. If the script runs and receives all arguments, you can
skip this section.

We will use Python as an example, but for other languages the process
is similar. Open cmd.exe as administrator and run:

    C:\>assoc .py

*   If the association doesn't exist (assoc prints nothing), create
    a new one:

        C:\>assoc .py=Python.File
        C:\>ftype Python.File="C:\Windows\py.exe" "%L" %*

*   If the association exists (assoc prints something like
    `.py=Python.File`), run ftype to see what command is used:

        C:\>ftype Python.File
        Python.File="C:\Windows\py.exe" "%L" %*

    If the command doesn't end with `%*`, fix it:

        C:\>ftype Python.File="C:\Windows\py.exe" "%L" %*

If the script still receives only one argument (path to the script),
this means that Windows actually use a different association for the
given extension and ignores the one set with assoc/ftype. To fix
that, open regedit and make sure the values of the following keys use
the correct path to the Python executable and end with `%*`:

    HKEY_CLASSES_ROOT\Applications\python.exe\shell\open\command
    HKEY_CLASSES_ROOT\py_auto_file\shell\open\command

A tip for Python users: note that in the examples above the
association uses Python Launcher (py.exe) rather than a concrete
Python executable (python.exe). This allows using [shebang][] lines to
select the Python version on per-script basis. For more information,
read [Using Python on Windows][].

[Using Python on Windows]: https://docs.python.org/3/using/windows.html


##### Hiding console window

Most scripting language interpreters for Windows are shipped with a
special version of the executable that doesn't show the console
window. For example, it's pyw.exe for Python and wlua.exe for Lua.

A special file association is usually added during installation of the
interpreter, so you can hide the console window by simply changing the
extension of the script. For example, Python scripts with the ".pyw"
extension are associated with pyw.exe instead of py.exe. Other
languages can have their own conventions, like ".wlua" for Lua
(wlua.exe). If such an association does not exist, create it manually
as described in the previous section.


## History tab

The History tab shows the history of recognized texts. A text is only
added here if the corresponding action is enabled in the
[Actions tab]. Every text in the list has a timestamp taken at the
moment you finish the selection.

You can export the history to a file in plain text, HTML, or JSON
format.


# Tweaking

This section describes how to change some settings that are not
available in the dpScreenOCR's interface.

dpScreenOCR saves settings in settings.cfg — a plain text file that
you can modify with any text editor. Depending on the platform, you
can find it in the following directories:

*   Windows: `%LOCALAPPDATA%\dpscreenocr`

    You can paste this path to the folder address bar of File Explorer
    to open it. `%LOCALAPPDATA%` is an environment variable which
    usually expands to `C:\Users\(your name)\AppData\Local\`

*   Unix-like systems: `~/.config/dpscreenocr`

Each line in settings.cfg contains an option as a key-value pair. A
value is a string, which, depending on the option, should represent a
boolean (`true` or `false`), number (like `10` or `-5`), file path,
arbitrary text, etc.

The value can contain the following escape sequences:

*   `\n` - line feed
*   `\r` - carriage return
*   `\t` - tabulation

Any other character preceded by `\` is kept as is. To preserve leading
spaces, escape the first one with `\`; to preserve trailing spaces,
either escape the last one or put `\` after it at the end of the line.

To reset an option to the default value, remove it from settings.cfg;
to reset all options, clear or delete the file. Be aware that
dpScreenOCR rewrites settings on exit, so make sure you close the
program before making changes.

Here is the list of options that can only be changed by editing the
settings file:

*   `action_copy_to_clipboard_text_separator` (`\n\n\n` by default)
    specify the separator for multiple texts for "Copy text to
    clipboard" action. This option only has effect if
    `ocr_allow_queuing` is enabled.

*   `history_wrap_words` (`true` by default) whether to break long
    lines of text in the history so that you don't have to scroll
    horizontally.

*   `hotkey_cancel_selection` (`Escape` by default) hotkey to cancel
    selection.

*   `ocr_allow_queuing` (`true` by default) allows to queue a new
    selection for recognition without waiting for the previous one to
    complete.

    If this option is enabled, the "Copy text to clipboard" action may
    receive several recognized text at once, in which case they will
    be joined together using `action_copy_to_clipboard_text_separator`
    as separator. If this option is disabled, pressing the hotkey will
    have no effect until the recognition is done.

*   `ui_tray_icon_visible` (`true` by default) whether to show an icon
    in the notification area.

*   `ui_window_minimize_on_start` (`false` by default) minimize window
    on start.

*   `ui_window_minimize_to_tray` (`false` by default) hide window to
    the notification area on minimizing. This option only has effect
    if `ui_tray_icon_visible` is enabled.


# Troubleshooting

This section contains the list of possible issues and their solutions.
If the solutions don't help, or you have an issue that is not listed
here, please report the problem on the [issue tracker][].

[Issue tracker]: https://github.com/danpla/dpscreenocr/issues

*   **Recognized text contains garbage**

    Make sure that you use the minimal set of [languages] needed to
    recognize the text. Don't enable languages just in case: this will
    dramatically reduce the accuracy of recognition.

*   **Pressing the [hotkey] has no effect**

    This hotkey is probably used by another program. Try to choose
    another key combination.

*   **"Run executable" has no effect**

    * Make sure that the "Run executable" entry contains either an
      absolute path to the executable, or just the name of the
      executable that resides in one of the paths of the PATH
      environment variable.

    * (Unix) Make sure you have execute permission. Run
      `chmod u+x executable`.

    * (Unix) If your executable is a script, make sure it starts with
      a proper [shebang][].

    * (Windows) Are you trying to use a batch file (".bat" or ".cmd")?
      This is not allowed for safety reasons. Please use Python or
      another scripting language instead.

*   **(Unix) Recognition is very slow**

    On some hardware, OpenMP multithreading in Tesseract 4 and 5
    results in dramatically slow recognition. The solution is to
    set the `OMP_THREAD_LIMIT` environment variable to `1` before
    running dpScreenOCR.

    Its not recommended to set `OMP_THREAD_LIMIT` globally because it
    will affect other programs that use OpenMP. Instead, create a
    helper script that sets the variable and then runs dpScreenOCR,
    e.g. via `env OMP_THREAD_LIMIT=1 dpscreenocr`. An even more
    convenient solution is to add an item to the applications menu by
    making a desktop entry that will either execute the helper script
    or will set `OMP_THREAD_LIMIT` itself:

    1.  Copy `dpscreenocr.desktop` from `/usr/share/applications/`
        or `/usr/local/share/applications/` to
        `~/.local/share/applications/` and rename it to
        `dpscreenocr-no-openmp.desktop`.

    2.  Open `dpscreenocr-no-openmp.desktop` in a text editor and
        change `Name=dpScreenOCR` to `Name=dpScreenOCR (No OpenMP)`
        and `Exec=dpscreenocr` to
        `Exec=env OMP_THREAD_LIMIT=1 dpscreenocr`. You can also tell
        `Exec` to launch a helper script instead, e.g.
        `Exec=/home/your_name/your_script.sh`.

    4.  If the entry does not appear in the applications menu, run
        `update-desktop-database ~/.local/share/applications` or
        re-login.

*   **(Unix) No languages**

    Make sure that the TESSDATA_PREFIX environment variable is either
    not set or points to the parent directory of your "tessdata"
    directory.

*   **(Windows) "Run executable" opens the script in a text editor**

    Create a file association as described in
    [Creating file associations].

*   **(Windows) "Run executable" runs the script without an
    argument**

    Make sure that the file association ends with `%*`. See
    [Creating file associations] for the details.
