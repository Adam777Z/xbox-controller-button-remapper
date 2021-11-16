# Xbox Controller button remapper

A small program that runs in the background and sends the configured key(s) when the Xbox/Guide button and/or Share button is pressed on the controller.

Works with the Xbox Series X|S Controller that has the Share button.

Make sure that the controller is updated to the latest firmware version.

Currently only works with this setting disabled:\
Settings - Gaming - Xbox Game Bar - Open Xbox Game Bar using this button on a controller: (X)

Supports up to 4 controllers.

Configure the program using the config.ini file.

## Requirements
- Xbox Controller
- Windows 10/11 64-bit
- [Microsoft Visual C++ Redistributable for Visual Studio 2019 x64](https://visualstudio.microsoft.com/downloads/#microsoft-visual-c-redistributable-for-visual-studio-2019)

## Download
Download the latest version on the [Releases](https://github.com/Adam777Z/xbox-controller-button-remapper/releases/latest) page.

## Run automatically at startup
Follow these instructions:
1. [Pin and unpin apps to the Start menu](https://support.microsoft.com/en-us/windows/pin-and-unpin-apps-to-the-start-menu-10c95188-5f75-bb6c-3fab-cfd678ac8476)
2. [Add an app to run automatically at startup in Windows 10](https://support.microsoft.com/en-us/windows/add-an-app-to-run-automatically-at-startup-in-windows-10-150da165-dcd9-7230-517b-cf3c295d89dd)

## Support and Feedback
Available under [Discussions](https://github.com/Adam777Z/xbox-controller-button-remapper/discussions).

## How to build
1. Have [SDL](https://github.com/libsdl-org/SDL) in the SDL folder:\
SDL\include\
SDL\VisualC\x64\Release
2. Open the Visual Studio Solution file and build

## Credits
- Thanks to Microsoft for the Xbox Controller
- [button_on_360_guide](https://www.reddit.com/r/emulation/comments/1goval/any_way_to_map_the_middle_xbox_360_button/camujj7/) (GitHub: [1](https://github.com/pinumbernumber/Xbox-360-Guide-Button-Remapper), [2](https://github.com/CautemocSg/xbox-360-guide-remapper))\
Thanks to the creator of the original program and the source code. I improved it.
- [SDL](https://github.com/libsdl-org/SDL)