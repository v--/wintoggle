[![Travis][]](https://travis-ci.org/v--/wintoggle) [![AUR][]](https://aur.archlinux.org/packages/wintoggle)

[Travis]: https://img.shields.io/travis/v--/wintoggle.svg?style=flat-square
[AUR]: https://img.shields.io/aur/version/wintoggle.svg?style=flat-square

wintoggle(1) -- a simple window focus toggler
=============================================

## SYNOPSIS

`wintoggle` [--help] <var>class</var> <var>executable</var>

## DESCRIPTION

A simple window focus toggler. Useful in conjunction with global hotkeys. It requires a window manager with EWMH support and is tested on OpenBox, KWin, Mutter and XFWM.

The program works in three steps and each one is executed only if the previous steps fail.

* __Step 1__: If the currently active window has a class name (actually WM_CLASS instance name) that equals <var>class</var>, the window is minimized.
* __Step 2__: If any window on the current display (from any screen or workspace) matches <var>class</var>, the window is moved to the current workspace and active.
* __Step 3__: <var>executable</var>, which defaults to <var>class</var> if not specified, is executed using `execlp`.

## EXAMPLES

Launch `xterm` if it is not already running

    $ wintoggle xterm
    [Step 1] The active window does not match class 'xterm'.
    [Step 2] The currently mapped windows do not match class 'xterm'.
    [Step 3] Executing 'xterm'.

Minimize `xterm` from inside xterm

    $ wintoggle xterm
    [Step 1] Minimizing window 14680086 (xterm).

Move `xterm` from another workspace and focus it

    $ wintoggle xterm
    [Step 1] The active window does not match class 'xterm'.
    [Step 2] Presenting window 14680086 (xterm) to the current workspace.

## OPTIONS

 * `-h`, `--help`:
    List available options and exit.

 * `-v`, `--version`:
    Print the program version and exit.


[SYNOPSIS]: #SYNOPSIS "SYNOPSIS"
[DESCRIPTION]: #DESCRIPTION "DESCRIPTION"
[EXAMPLES]: #EXAMPLES "EXAMPLES"
[OPTIONS]: #OPTIONS "OPTIONS"


[-]: -.html
