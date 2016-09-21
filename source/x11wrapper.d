import std.typecons;
import std.conv;
import x11.X;
import x11.Xlib;
import x11.Xutil;
import std.string : toStringz, fromStringz;
import utils;

enum SOURCE_INDICATION = 2L; // Pager

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

void focusWindow(WindowMatch match)
{
    sendEvent(match, "_NET_ACTIVE_WINDOW", [SOURCE_INDICATION, CurrentTime]);
    sendEvent(match, "_NET_WM_CURRENT_DESKTOP", [getActiveDesktop(), SOURCE_INDICATION]);
}

private:

void sendEvent(WindowMatch match, string messageType, long[] data)
{
    XEvent event;

    event.type = ClientMessage;
    event.xclient.display = display;
    event.xclient.window = match.window;
    event.xclient.message_type = XInternAtom(display, messageType.toStringz, false);
    event.xclient.format = 32;
    event.xclient.data.l[0..data.length] = data[];

    XSendEvent(
        display,
        RootWindow(display, match.screen),
        false,
        SubstructureNotifyMask | SubstructureRedirectMask,
        &event
    );

    XSync(display, match.screen);
}

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
    return getClassName(window) == className && ~getDesktop(window);
}

int errorHandler(XErrorEvent* err)
{
    int length;
    char* buffer;
    XGetErrorText(display, err.error_code, buffer, length);
    error(buffer.to!string);
    return 0;
}

long getDesktop(Window window)
{
    return getWindowProperty(window, "_NET_WM_DESKTOP");
}

long getActiveDesktop()
{
    // The the active desktop for the first screen
    return getWindowProperty(RootWindow(display, 0), "_NET_WM_CURRENT_DESKTOP");
}

long getWindowProperty(Window window, string property)
{
    long result;
    Atom atom = XInternAtom(display, property.toStringz, false);
    Atom actualType;

    int actualFormat;
    ulong nItems, bytesAfter;
    ubyte* prop;

    XGetWindowProperty(
        display,
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

    if (prop is null) {
        return -1;
    }

    result = *cast(uint*)prop;
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
