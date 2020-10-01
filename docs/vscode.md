# Using VSCode and Platform IO with your Tympan

In addition to using the Arduino/Teensyduino IDE for working your Tympan, it is possible to use VSCode and PlatformIO as a development environment. It does, however, require some customizations to your `platformio.ini` file that might not be immediately obvious. The process for configuring VSCode and Platform IO (after you have followed the main instructions in the readme) is roughly as follows:

1. If you do not already have it, [download VSCode](https://code.visualstudio.com/download)
2. Install the [PlatformIO IDE plugin in VSCode](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) (note: if you use Anaconda on a Mac, you will likely need to update or reinstall Anaconda)
3. Create a new project in the PlatformIO IDE (choose the `teensy36` board)
4. Add the following three libraries to the lib_deps option: `Tympan_Library`, `https://github.com/PaulStoffregen/Audio.git`, and `https://github.com/PaulStoffregen/SD.git` (note: these are the same libraries required by the Tympan Library itself)
5. Put the path to the `libraries` directory you located in the "Installing the Tympan Library" section of the [readme](../readme.md) in the `lib_extra_dirs` option
6. Make sure to follow the instructions that PlatformIO gives you to convert your `.ino` file into a `.cpp` file (basically renaming the file and adding some additional `#include` directives)
7. Connect your Tympan to your computer and turn it on
8. Click the "Run" (right-facing arrow) button in the status bar to run your sketch on the Tympan