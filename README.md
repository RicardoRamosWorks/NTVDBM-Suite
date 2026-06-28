# NTVDBM Suite

![Image](https://raw.githubusercontent.com/RicardoRamosWorks/ricardoramosworks.github.io/refs/heads/main/Images/ntVDBM.png)

NT Virtual DosBox Machine Suite: This project aims to transparently replace NTVDM with DOSBox. It was developed for use on UEFI-based Windows XP installations, serving as a solution for playing Doom.

My handler executes MS-DOS applications without requiring NTVDM (since Apple TV does not support BIOS). Additionally, it creates a per-application configuration file (config files are stored in the `system32` folder, but the system creates a shortcut to them inside the app/game folder). Configuration files (and icons) are generated using a CRC16 hash to ensure unique identification for each game.

There is also my "shortcut creation" system. Unfortunately, due to limitations in how Windows handles DOS applications, it is impossible to create a shortcut to a 16-bit executable without it turning into a PIF (*Processed Intent File*). PIF files rely on NTVDM; this forced me to write a completely new solution from scratch—which turned out to be a good thing, as it allowed me to implement improvements. The global context menu now includes an option to create a desktop shortcut for the game. This shortcut launches NTVDBM or Winbox.exe, passing the target object and its corresponding configuration file as arguments. Furthermore, the system automatically assigns an icon if a matching one exists in the icons folder or the directory containing the executable.

To clarify: there is an `icons` folder within `system32` named `gameicons`; inside, there is a `default***.ico` file and icons for some games I have already configured. If you create a shortcut for `doom.exe`, the handler copies `default***.ico` to `doom+hash.ico`. The hash is calculated based on the executable file, allowing icons to be shared among users to build a large icon library. I am creating icons for the games I own and play, and I plan to make them available in a collective pack. If an icon matching the game's name and hash already exists in the icon folder, the shortcut will be created using the correct icon! Additionally, if an .ico file is already present in the game folder, the handler will associate that icon with the shortcut. This works for everything: MS-DOS, Win16, and Win32 applications.

When you click on a game, my custom DOSBox build is launched with all the necessary arguments and paths to run it correctly. When the game ends, it automatically receives an exit command and returns to the desktop.

The system features a refined executable detection mechanism that accurately distinguishes between 32-bit executables, 16-bit Windows (Windows 3.1) executables, and 16-bit MS-DOS executables.

Currently, all MS-DOS executables and related configurations work perfectly. 16-bit executables are redirected to Winbox—which is also based on DOSBox but includes various modifications to serve as a dedicated tool for Windows 3.1. If Winbox is launched with an executable as an argument, it quickly starts the 16-bit environment and launches the 16-bit executable within that window. The host's C:\ drive is correctly mapped as the C: drive, allowing you to install games using 16-bit installers. The W: drive is where Windows 3.1 runs. If Winbox is launched without arguments, Program Manager starts, allowing you to use Windows 3.1 normally. Windows 3.1 is located at c:\windows\system32\winbox\ on the host system. I recommend using a solid Windows 3.1 installation—fully updated with components like WinG, Video for Windows, and QuickTime—to ensure compatibility with early FMV games.

I created this setup based on Windows 3.11, but unfortunately, I cannot distribute it with my project; even though it is considered abandonware, Microsoft still holds the rights.

The project includes a selection of pre-configured icons for some classic games I own. There is no guarantee that your `doom.exe` will display the correct icon, as the icon is linked to the executable via a checksum. Therefore, if your `doom.exe` is from a different version than the one I used, you will need to create an icon for your specific executable.

## HOW TO USE:

Download and extract the files to the `System32` folder of your Windows XP installation. Merge the `install.reg` registry file (simply double-click it), and you're all set.

There are two other registry files: one to enable the handler and another to disable it; these act as a system-wide on/off switch. Certain programs might not work correctly while my handler is intercepting calls, so you may need to disable it before running specific software (though I have refined it significantly to avoid issues during my testing).

## Creating Icons

"Creating" might not be the perfect word, but I couldn't think of a better one....better. You don't necessarily need to create an icon from scratch. When creating a shortcut, the handler copies the `default_dos.ico`, `default_win16.ico`, or `default_win32.ico` file to a new file; the filename will match the executable's name plus a hash. If a `doom+hash.ico` file already exists in the folder but your Doom shortcut didn't get the correct icon, it means your executable version is different and generated a distinct hash. To fix this, simply locate the generic `doom+hash.ico` file, copy the icon containing the desired image, and rename it accordingly.

## Compatibility

All the games I tested—released between 1987 and 1999—ran perfectly (meaning there were no issues with the handler); I believe I have achieved 100% compatibility with any game designed to run in DOSBox.