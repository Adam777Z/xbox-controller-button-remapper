# Xbox Controller Button Remapper

A small program that runs in the background and sends the configured key(s) when the Xbox button and/or Share button is pressed on the controller.

Supports capturing screenshots and videos directly without any other program involved so the button prompts do not change in the game after pressing the Xbox/Share button on the controller to take the screenshot/video.\
Can also capture screenshots and videos using keys on the keyboard.

Can also open any file using keyboard key and/or controller button so the button prompts do not change in the game.

Configure the program using the config.ini file.

Works with the Xbox Series X|S Controller that has the Share button.

Make sure that the controller is updated to the latest firmware version. (Instructions: [Update your Xbox Wireless Controller](https://support.xbox.com/en-US/help/hardware-network/controller/update-xbox-wireless-controller))

Remapping the Xbox button only works when the **Open Game Bar using Xbox button on a controller** setting is disabled:\
[Enable or Disable Open Game Bar using Xbox button on Controller in Windows 10](https://www.tenforums.com/tutorials/138967-enable-disable-open-xbox-game-bar-using-controller-windows-10-a.html)\
[Enable or Disable Open Game Bar using Xbox button on Controller in Windows 11](https://www.elevenforum.com/t/enable-or-disable-open-game-bar-using-xbox-button-on-controller-in-windows-11.4290/)

Remapping the Share button only works when the **Windows Game Recording and Broadcasting** features are disabled:\
[Enable or Disable Windows Game Recording and Broadcasting in Windows 10](https://www.tenforums.com/tutorials/51180-enable-disable-windows-game-recording-broadcasting-windows-10-a.html)\
[Enable or Disable Windows Game Recording and Broadcasting in Windows 11](https://www.elevenforum.com/t/enable-or-disable-game-recording-for-captures-in-windows-11.17611/)

## Requirements
- Xbox Controller
- Windows 10/11 64-bit
- [Microsoft Visual C++ Redistributable for Visual Studio 2022 x64](https://visualstudio.microsoft.com/downloads/#microsoft-visual-c-redistributable-for-visual-studio-2022)

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
Thanks to the creator of the original program and the source code so it could be improved. It was improved.
- [SDL](https://github.com/libsdl-org/SDL)
- Desktop Duplication API for capturing screenshots and videos ([1](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api), [2](https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/DXGIDesktopDuplication), [3](https://github.com/microsoftarchive/msdn-code-gallery-microsoft/tree/master/Official%20Windows%20Platform%20Sample/DXGI%20desktop%20duplication%20sample), [4](https://www.codeproject.com/Tips/1116253/Desktop-Screen-Capture-on-Windows-via-Windows-Desk), [5](https://github.com/GERD0GDU/dxgi_desktop_capture), [6](https://github.com/WindowsNT/ScreenCapture), [7](https://www.codeproject.com/Articles/5256890/ScreenCapture-Single-Header-DirectX-Library-with-H))