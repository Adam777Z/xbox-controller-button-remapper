# Xbox Controller Button Remapper

A small program that runs in the background and sends the configured key(s)/triggers action when the Xbox button and/or Share button is pressed on the controller.

Supports capturing screenshots directly without any other program involved so the button prompts do not change in the game after pressing the Xbox/Share button on the controller to take the screenshot.\
Can also capture screenshots using keys on the keyboard.

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

## Features
- Remap Xbox button and/or Share button on the controller to keyboard key(s) and/or function(s)
- Xbox/Share button long press (hold and release) remapping
- Multiple controllers are supported
- Multiple keyboard keys are supported
- Wait before sending key(s) (delay)
- Wait before releasing key(s) (duration)
- Capture screenshots using keyboard key and/or controller button
- Open file using keyboard key and/or controller button
- Mute/unmute default recording/input device (microphone) (toggle and push-to-talk) using controller button
- Configure the program using the config.ini file

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
2. Open the Visual Studio Solution file
3. Install packages in [NuGet Package Manager](https://learn.microsoft.com/en-us/nuget/consume-packages/install-use-packages-visual-studio)
4. [Build Solution](https://learn.microsoft.com/en-us/visualstudio/ide/building-and-cleaning-projects-and-solutions-in-visual-studio)

## Credits
- Thanks to Microsoft for the Xbox Controller
- [button_on_360_guide](https://www.reddit.com/r/emulation/comments/1goval/any_way_to_map_the_middle_xbox_360_button/camujj7/) (GitHub: [1](https://github.com/pinumbernumber/Xbox-360-Guide-Button-Remapper), [2](https://github.com/CautemocSg/xbox-360-guide-remapper))\
Thanks to the creator of the original program and the source code that allowed it to be improved.
- [SDL](https://github.com/libsdl-org/SDL)
- Windows.Graphics.Capture APIs for capturing screenshots ([1](https://learn.microsoft.com/en-us/uwp/api/windows.graphics.capture), [2](https://learn.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture), [3](https://blogs.windows.com/windowsdeveloper/2019/09/16/new-ways-to-do-screen-capture/), [4](https://github.com/robmikh/Win32CaptureSample), [5](https://github.com/robmikh/ScreenshotSample))