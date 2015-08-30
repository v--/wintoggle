import std.getopt;
import x11wrapper;
import utils;

void main(string[] args)
{
    string className, executable;
    WindowMatch match;

    try getopt(
            args,
            "h|help", &displayHelp,
            );

    catch (GetOptException e)
        error(e.msg);

    if (args.length < 2 || args.length > 3)
        displayHelp();

    className = args[2];
    executable = args[args.length == 3 ? 3 : 2];

    match = checkActiveHierarchy(className);

    if (match.window)
    {
        minimizeWindow(match);
        writefln("[Stage 1] Minimized window number %d (%s)", match.window, className);
        return;
    }

    writefln("[Stage 1] The focused window (nor any of its parents) do not match class %s", className);
    match = findWindowByClassName(className);

    if (match.window)
    {
        focusWindow(match);
        writefln("[Stage 2] Focused on window number %d (%s)", match.window, className);
        return;
    }

    writefln("[Stage 2] The currently mapped windows do not match class %s", className);
    launchProcess(executable);
}
