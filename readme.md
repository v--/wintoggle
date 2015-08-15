The script minimizes/maximizes windows, based on the following algorithm:
* A search is initiated for a certain window class
* If a single instance of the window class exists, it's minimize/maximize state is toggled
* If multiple instances of the window class exist, on each run the script switches between them
* If the window class isn't matched, an attempt is made to launch it (see bellow)

CLI parameters:
* Parameter 1: Window class; Parameter 2: Bin executable (optional)
* Parameter 2 is launched if the window class is not detected
  If omitted, the script attempt to launch the program using it's class name

I find the script very useful in conjunction with global hotkeys
