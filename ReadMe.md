# Runner

This is a simple program to replace/imitate the system Start/Run dialog.

When you disable the Win+? hotkeys via registry, Win+R gets disabled too. I'm using this setting, but I find Win+R very useful, and didn't want to lose it. That's why I wrote this program.

In addition it has an option to run the program with Administrator permissions, which the standard dialog lacks, so this is another reason to use it. However, if you didn't disable Win+R, Runner will not be able to start, failing to register this shortcut. There is a plan to add an option for using another hotkey, but it's not implemented yet.

## Installation

This is a portable application. Just unpack the archive, and start `Runner.exe`. If you plan to use it constantly, it's a good idea to make this program to run automatically when computer boots. There is no built-in option for that, you need to do it manually by placing a LNK shortcut in your Startup folder, or adding a registry entry.

You can edit `Runner.ini` to change the language (the respective LNG file must be present in the `Language` subdirectory).

## Using

The program is mostly self-explanatory; if you have ever used the Start/Run dialog, you should not have any problems with Runner. When you press Win+R, the Run dialog appears, where you can specify the command you want to run. Optionally you can tick the `As Administrator` option, and the command will be started with the appropriate privileges (an UAC consent or administrator credentials request will appear, if not disabled in the Windows settings).

## Building

To compile the program from source, you can use Microsoft Visual Studio 2017. Just open the project and build it for the 32- or 64-bit architecture. No additinal librarries are required.

You might need to change the Win10 SDK version in the project settings to the one you have installed, though.
