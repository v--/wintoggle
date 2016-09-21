public import std.stdio: writefln;
import core.stdc.stdlib: exit;
import std.process;
import std.string;
import utils;

void displayHelp()
{
    import std.stdio: write;
    write(import ("README.md"));
    exit(0);
}

void error(string msg)
{
    writefln("Error: %s", msg);
    exit(1);
}

void launchProcess(string executable)
{
    try execute(executable.split(' '));

    catch (Exception e)
    {
        writefln("[Stage 3] %s", e.msg);
        exit(1);
    }

    writefln("[Stage 3] Launched ", executable);
}
