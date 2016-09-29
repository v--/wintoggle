#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define WT_VERSION "3.0.1"

#define WT_HELP_MESSAGE "\
This is a short usage summary. See wintoggle(1) for more information. \n\
\n\
Usage: wintoggle [--help] [--version] class [executable] \n\
\n\
Positional arguments: \n\
 class          The window class to match. \n\
 executable     The executable to launch. It defaults to the window class. \n\
\n\
Optional arguments: \n\
 -h, --help     List available options and exit. \n\
 -v, --version  Print the program version and exit."

#define SOURCE_INDICATION 2L // Pager

#define DEFAULT_WT_WINDOW_INFO (WTWindowInfo) { .window = 0, .screen = -1, .class_name = NULL };

typedef struct
{
    const Window window;
    const char* class_name;
    const int screen;
} WTWindowInfo;

typedef struct
{
    const char* class_name;
    const char* executable;
} WTParams;

typedef enum
{
    WT_PARAMS_OK,
    WT_PARAMS_INVALID,
    WT_PARAMS_EXIT
} WTParamsStatus;

long
wt_get_long_window_property(Display* display, Window window, const char* property)
{
    long result;
    unsigned char* value = NULL;
    Atom atom = XInternAtom(display, property, false);

    // Assume that the result is an int and ignore the following properties
    Atom actual_type;
    int actual_format;
    unsigned long n_items;
    unsigned long bytes_after;

    if (XGetWindowProperty(
        display,
        window,
        atom,
        0,
        1,
        false,
        AnyPropertyType,
        &actual_type,
        &actual_format,
        &n_items,
        &bytes_after,
        &value
    ) != Success) {
        fprintf(stderr, "Could not acquire property '%s' for window %lu.\n", property, window);
        return -1;
    }

    if (value == NULL) {
        return -1;
    }

    result = *(long*)value;
    XFree(value);
    return result;
}

int
wt_get_desktop(Display* display, Window window)
{
    return wt_get_long_window_property(display, window, "_NET_WM_DESKTOP");
}

bool
wt_get_window_name(Display* display, Window window, char** class_name_return)
{
    XClassHint class_hint;

    if (XGetClassHint(display, window, &class_hint) == 0)
    {
        // This condition fails for most windows so don't print an error message
        // fprintf(stderr, "Could not get the window class hint of window %lu.\n", window);
        return false;
    }

    int name_length = strlen(class_hint.res_name);
    class_name_return[0] = (char*) malloc(name_length + 1);
    memcpy(*class_name_return, class_hint.res_name, name_length);
    class_name_return[0][name_length] = '\0';

    XFree(class_hint.res_name);
    XFree(class_hint.res_class);

    return true;
}

bool
wt_window_matches(Display* display, Window window, const char* class_name)
{
    char* class_name_buffer = NULL;

    if (!wt_get_window_name(display, window, &class_name_buffer))
        return false;

    bool result = strcmp(class_name_buffer, class_name) == 0;
    free(class_name_buffer);
    return result;
}

WTWindowInfo
wt_search_by_class_name_recursively(
    Display* display,
    int screen,
    const char* class_name,
    Window root,
    Window window
)
{
    Window parent; // This variable is not actually used.
    Window* children = NULL;
    unsigned int child_count;

    if (wt_window_matches(display, window, class_name))
        return (WTWindowInfo) { .window = window, .screen = screen, .class_name = class_name };

    if (XQueryTree(display, window, &root, &parent, &children, &child_count) == 0)
        fprintf(stderr, "Error: Could not acquire the children of window %lu\n.", parent);

    for (size_t i = 0; i < child_count; i++)
    {
        WTWindowInfo match = wt_search_by_class_name_recursively(
            display,
            screen,
            class_name,
            root,
            children[i]
        );

        if (match.window > 0)
        {
            XFree(children);
            return match;
        }
    }

    XFree(children);
    return DEFAULT_WT_WINDOW_INFO;
}

WTWindowInfo
wt_find_window_by_class_name(Display* display, const char* class_name)
{
    for (int screen = 0; screen < XScreenCount(display); screen++)
    {
        Window root = XRootWindow(display, screen);
        WTWindowInfo match = wt_search_by_class_name_recursively(
            display,
            screen,
            class_name,
            root,
            root
        );

        if (match.window > 0)
            return match;
    }

    return DEFAULT_WT_WINDOW_INFO;
}

bool
wt_send_event(
    Display* display,
    Window target,
    WTWindowInfo info,
    const char* message_type,
    size_t data_size,
    const long* data
)
{
    XEvent event;
    memset(&event, 0, sizeof(event));
    memcpy(event.xclient.data.l, data, data_size * sizeof(long) / sizeof(char));

    event.type = ClientMessage;
    event.xclient.format = 32;
    event.xclient.window = info.window;
    event.xclient.display = display;
    event.xclient.message_type = XInternAtom(display, message_type, false);

    if (XSendEvent(
        display,
        target,
        false,
        SubstructureNotifyMask | SubstructureRedirectMask,
        &event
    ) == 0)
    {
        fprintf(stderr, "Could not send a '%s' event to window %lu (%s).\n", message_type, info.window, info.class_name);
        return false;
    }

    return true;
}

bool
wt_minimize_window(Display* display, WTWindowInfo info)
{
    if (XIconifyWindow(display, info.window, info.screen) == 0)
    {
        fprintf(stderr, "Could not iconify window %lu (%s).\n", info.window, info.class_name);
        return false;
    }

    XSync(display, false);
    return true;
}

bool
wt_focus_window(Display* display, WTWindowInfo info)
{
    Window root = XDefaultRootWindow(display);
    long active_desktop = wt_get_long_window_property(display, root, "_NET_CURRENT_DESKTOP");

    if (
        !wt_send_event(
            display, root, info, "_NET_WM_DESKTOP", 2,
            (long[]) { active_desktop, SOURCE_INDICATION }
        ) ||
        !wt_send_event(
            display, root, info, "_NET_ACTIVE_WINDOW", 2,
            (long[]) { SOURCE_INDICATION, CurrentTime }
        )
    )
        return false;

    XSync(display, false);
    return true;
}

WTWindowInfo
wt_active_window_info(Display* display, char** class_name_return)
{
    Window root = XDefaultRootWindow(display);
    class_name_return[0] = NULL;
    long active_window_id = wt_get_long_window_property(display, root, "_NET_ACTIVE_WINDOW");
    int screen = 0;

    if (active_window_id <= 0)
        return DEFAULT_WT_WINDOW_INFO;

    Window active = (Window)active_window_id;
    XWindowAttributes attributes;
    memset(&attributes, 0, sizeof(attributes));
    XGetWindowAttributes(display, active, &attributes);
    screen = XScreenNumberOfScreen(attributes.screen);
    wt_get_window_name(display, active, class_name_return);

    return (WTWindowInfo) {
        .window = active,
        .screen = screen,
        .class_name = *class_name_return
    };
}

WTParamsStatus
wt_params_populate(int arg_count, const char** args, WTParams* params)
{
    WTParams result; // Use a working copy and copy it to <params> only on success
    memset(&result, 0, sizeof(result));
    bool after_split = false;

    for (size_t i = 0; i < arg_count; i++)
    {
        const char* arg = args[i];

        if (after_split)
            goto notopt;

        if (strcmp(arg, "--") == 0)
            after_split = true;
        else if (arg[0] == '-')
        {
            if (arg[1] == '\0')
                continue;
            else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0)
            {
                puts(WT_VERSION);
                return WT_PARAMS_EXIT;
            }
            else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
            {
                puts(WT_HELP_MESSAGE);
                return WT_PARAMS_EXIT;
            }
            else
            {
                fprintf(stderr, "Invalid option '%s'.\n", arg);
                return WT_PARAMS_INVALID;
            }
        }
        else
            goto notopt;

        continue;
        notopt:

        if (result.class_name == NULL)
            result.class_name = arg;
        else if (result.executable == NULL)
            result.executable = arg;
        else
        {
            fprintf(stderr, "Excess argument '%s'.\n", arg);
            return WT_PARAMS_INVALID;
        }
    }

    if (result.class_name == NULL)
    {
        fputs("No class name specified.\n", stderr);
        return WT_PARAMS_INVALID;
    }

    if (result.executable == NULL)
        result.executable = result.class_name;

    memcpy(params, &result, sizeof(WTParams));
    return WT_PARAMS_OK;
}

int
main(int argc, const char** argv)
{
    WTParams params;

    switch (wt_params_populate(argc - 1, &argv[1], &params))
    {
    case WT_PARAMS_INVALID: return 1;
    case WT_PARAMS_EXIT: return 0;
    case WT_PARAMS_OK: break;
    }

    Display* display = XOpenDisplay(NULL); // Open the default display

    if (display == NULL)
    {
        fputs("Error: Could not open the default display.\n", stderr);
        return 1;
    }

    WTWindowInfo match = wt_find_window_by_class_name(display, params.class_name);

    // Step 1
    {
        char* class_name_buffer = NULL;
        Window root = XDefaultRootWindow(display);
        long active_window_id = wt_get_long_window_property(display, root, "_NET_ACTIVE_WINDOW");

        // The initial implementation compared the active window class name with the class_name param
        // but sometimes, after closing the last window, OpenBox did not update _NET_SHOWING_DESKTOP
        // but left the root window with an invalid _NET_ACTIVE_WINDOW.
        if (match.window > 0 && match.window == active_window_id)
        {
            WTWindowInfo active_info = wt_active_window_info(display, &class_name_buffer);
            printf("[Step 1] Minimizing window %lu (%s).\n", active_info.window, active_info.class_name);
            bool status = wt_minimize_window(display, active_info);
            free(class_name_buffer);
            XCloseDisplay(display);
            return status;
        }
        else
        {
            printf("[Step 1] The active window does not match class '%s'.\n", params.class_name);
            free(class_name_buffer);
        }
    }

    // Step 2
    {
        if (match.window > 0)
        {
            printf("[Step 2] Presenting window %lu (%s) to the current workspace.\n", match.window, params.class_name);
            bool status = wt_focus_window(display, match);
            XCloseDisplay(display);
            return status;
        }
        else
            printf("[Step 2] The currently mapped windows do not match class '%s'.\n", params.class_name);
    }

    // Step 3
    {
        XCloseDisplay(display);
        printf("[Step 3] Executing '%s'.\n", params.executable);
        fflush(stdout);

        if (execlp(params.executable, params.executable, NULL) == -1)
        {
            perror("Could not execute the specified program");
            return 1;
        }

        return 0;
    }
}
