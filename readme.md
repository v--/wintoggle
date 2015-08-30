# wintoggle

A simple window focus toggler. Useful in conjunction with global hotkeys.

The program works in the following fashion:

* [Stage 1]: The currently focused mapped window (and all its mapped parents) are matched against a class name. If any of them matches, the window is minimized and the program terminates. Otherwise the program proceeds to Stage 2.

* [Stage 2]: All mapped windows on the current display are scanned. The first window that matches is maximized. If nothing is matched, the program proceeds to Stage 3

* [Stage 3]: An attempt is made to launch the executable that is supposed to have the specified window class name.

CLI arguments:

```
wintoggle (class) [executable] [-h/--help]

* class: The window class to match
* executable (optional): The executable to launch on failure. If it is omitted, the class name is used
* -h / --help: Display this message
```
