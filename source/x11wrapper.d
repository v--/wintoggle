import std.typecons;
import std.conv;
import x11.X;
import x11.Xlib;
import x11.Xutil;
import utils;

struct WindowMatch
{
    Window window;
    int screen;
}

WindowMatch findWindowByClassName(string className)
{
    WindowMatch match;

    foreach (screen; 0..ScreenCount(display))
    {
        auto rootWindow = RootWindow(display, screen);

        if (windowMatches(rootWindow, className))
            return WindowMatch(rootWindow, screen);

        match = matchClassRecursive(
            WindowTree(rootWindow).children,
            screen,
            className
        );

        if (!match.window)
            break;
    }

    return match;
}

WindowMatch checkActiveHierarchy(string className)
{
    auto window = getFocusedWindow;

    while (window)
    {
        if (windowMatches(window, className))
            return WindowMatch(window);

        window = WindowTree(window).parent;
    }

    return WindowMatch();
}

void minimizeWindow(WindowMatch match)
{
    XIconifyWindow(display, match.window, match.screen);
    XSync(display, match.screen);
}

// This function is taken from https://github.com/jordansissel/xdotool
void focusWindow(WindowMatch match)
{
    XEvent event;

    event.type = ClientMessage;
    event.xclient.display = display;
    event.xclient.window = match.window;
    event.xclient.message_type = XInternAtom(display, "_NET_ACTIVE_WINDOW", false);
    event.xclient.format = 32;
    event.xclient.data.l[0] = 2L; /* 2 == Message from a window pager */
    event.xclient.data.l[1] = CurrentTime;

    XSendEvent(
        display,
        RootWindow(display, match.screen),
        false,
        SubstructureNotifyMask | SubstructureRedirectMask,
        &event
    );

    XSync(display, match.screen);
}

private:

Display* display;

struct WindowTree
{
    Window root;
    Window parent;
    Window[] children;

    this(Window window)
    {
        Window* childrenPointer;
        uint childCount;
        XQueryTree(display, window, &root, &parent, &childrenPointer, &childCount);
        children = childrenPointer[0..childCount];
    }
}

int getScreen(Window window)
{
    XWindowAttributes attrs;
    XGetWindowAttributes(display, window, &attrs);
    return XScreenNumberOfScreen(attrs.screen);
}

bool windowMatches(Window window, string className)
{
    return getClassName(window) == className && hasDesktop(window);
}

int errorHandler(XErrorEvent* err)
{
    int length;
    char* buffer;
    XGetErrorText(display, err.error_code, buffer, length);
    error(buffer.to!string);
    return 0;
}

bool hasDesktop(Window window)
{
    bool result;
    Atom atom = XInternAtom(display, "_NET_WM_DESKTOP", false),
         actualType; // not actually used

    int actualFormat; // not actually used
    ulong nItems, bytesAfter; // not actually used
    ubyte* prop; // not actually used

    XGetWindowProperty(display,
                       window,
                       atom,
                       0,
                       ushort.max,
                       false,
                       AnyPropertyType,
                       &actualType,
                       &actualFormat,
                       &nItems,
                       &bytesAfter,
                       &prop
   );

    result = prop !is null;
    XFree(prop);

    return result;
}

string getClassName(Window window)
{
    string result;
    XClassHint classHint;
    XGetClassHint(display, window, &classHint);
    result = classHint.res_name.to!string;
    XFree(classHint.res_name);
    XFree(classHint.res_class);
    return result;
}

Window getFocusedWindow()
{
    Window focused;
    int revertState;
    XGetInputFocus(display, &focused, &revertState);
    return focused;
}

WindowMatch matchClassRecursive(Window[] windows, int screen, string className)
{
    WindowMatch match;

    foreach (window; windows)
        if (windowMatches(window, className))
            return WindowMatch(window, screen);

    foreach (window; windows)
    {
        match = matchClassRecursive(
            WindowTree(window).children,
            screen,
            className
        );

        if (match.window)
            return match;
    }

    return match;
}

shared static this()
{
    display = XOpenDisplay(null);

    if (display is null)
        error("XOpenDisplay failed");
}

shared static ~this()
{
    XCloseDisplay(display);
}
