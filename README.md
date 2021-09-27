# Xbox Controller button remapper

A small program that runs in the background and sends the configured key(s) when the Xbox/Guide button and/or Share button is pressed on the controller.

Works with the Xbox Series X|S Controller that has the Share button.

Make sure that the controller is updated to the latest firmware version.

Currently only works with this setting disabled:\
Windows 10 Settings - Gaming - Xbox Game Bar - Open Xbox Game Bar using this button on a controller: (X)

Configure the program using the config.ini file.

## Requirements
- Xbox Controller
- Windows 10 64-bit
- [Microsoft Visual C++ Redistributable for Visual Studio 2019 x64](https://visualstudio.microsoft.com/downloads/#microsoft-visual-c-redistributable-for-visual-studio-2019)

## Download
Download the latest version on the [Releases](https://github.com/Adam777Z/xbox-controller-button-remapper/releases/latest) page.

## How to build
1. Have [SDL](https://github.com/libsdl-org/SDL) in the SDL folder:\
SDL\include\
SDL\VisualC\x64\Release
2. Open the Visual Studio Solution file and build

## Credits
- [button_on_360_guide](https://www.reddit.com/r/emulation/comments/1goval/any_way_to_map_the_middle_xbox_360_button/camujj7/) ([GitHub](https://github.com/pinumbernumber/Xbox-360-Guide-Button-Remapper))
- [SDL](https://github.com/libsdl-org/SDL)