# UE4 Port of DualSense on Windows [UE4 Port of Ohjurot/DualSense-Windows](https://github.com/Ohjurot/DualSense-Windows)
![](https://raw.githubusercontent.com/Ohjurot/DualSense-Windows/main/Doc/GitHub_readme/header.png)

Windows API for the PS5 DualSense controller.  Written in C++ for Unreal Engine projects. This API will help you using the DualSense controller in your Windows Unreal Engine 4 Applications / Projects.

> :exclamation: ​Warning: The current release state is still a preview release. The library may not work as intended!

## Using the API

1. Download the latest release found at the [Release Page](https://github.com/Ohjurot/DualSense-Windows/releases)
2. Unzip the archive to your computer
3. Read the `DualSenseWindows.pdf`  PDF documentation to get the specific information for your current release

If you don't want to mess your time documentation - this is the minimal example on how to use the library:

## Known issues 

- When the controller being shut down while connected via Bluetooth (Holding the PS button). The lib will encounter a dead lock within `getDeviceInputState(...)` call. The function will return as soon as the controller is getting reconnected. Not encountering over USB, over USB the expected `DS5W_E_DEVICE_REMOVED` error is returned. 

## Special thanks to

Ludwig Füchsl: https://github.com/Ohjurot

Those thanked in DualSense-Windows that made this project possible:

- The GitHub community on this project and https://github.com/Ryochan7/DS4Windows/issues/1545
- https://www.reddit.com/r/gamedev/comments/jumvi5/dualsense_haptics_leds_and_more_hid_output_report/
- https://gist.github.com/dogtopus/894da226d73afb3bdd195df41b3a26aa
- https://gist.github.com/Ryochan7/ef8fabae34c0d8b30e2ab057f3e6e039
- https://gist.github.com/Ryochan7/91a9759deb5dff3096fc5afd50ba19e2



[Important Informations about Trademarks](https://github.com/Ohjurot/DualSense-Windows/blob/main/TRADEMARKS.md)
