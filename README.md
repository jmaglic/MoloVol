# CaVol<img src="https://user-images.githubusercontent.com/65410083/89782147-00a79400-db15-11ea-8ca6-d545902d1aff.png" width="250" ALIGN="right">

Cavol is a free, cross-plattform, scientific software for volume and surface computations of single molecules and crystallographic unit cells.

## Utility
With the CaVol software we aim to combine an intuitive and easy-to-use user interface with powerful computing algorithms. We want the software to serve a broad range of scientist by being easily accessible on all common platforms.

The CaVol software, along with many of its features, is currently in early development. The software is being actively developed on Windows 10 and macOS Catalina.

## Getting Started

Dependencies:
- wxWidgets 3.1.4 (https://www.wxwidgets.org)

### On macOS

This guide explains how to set up a development environment for macOS in the terminal. You may need to first install Xcode command line tools, if you haven't already. This should be as easy as running the following command in the terminal.

`$ xcode-select --install`

Afterwards you should have gcc installed. You can check whether gcc is installed by requesting the version.

`$ gcc -v`

#### Installing wxWidgets 3.1.4

To install wxWidgets visit https://www.wxwidgets.org and download the source files for Mac. Unzip the download file and place it in a directory of your choice, such as your user directory `Users/myname/wx`. Then enter that directory.

`$ cd wx/wxWidgets-3.1.4`

In order to compile wxWidgets and not risk overwriting it upon recompilation, create a new directory and enter it.

`$ mkdir build-debug`
`$ cd build-debug`

Afterwards run the configuration executable in the parent folder with the following flags: Enable building for debugging, disable shared to obtain a static library, and enable unicode support.

`$ ../configure --enable-debug --disable-shared --enable-unicode`

Now you're ready to compile. Simply run:

`$ make`

This step will take a while. Once compilation has finished, you can place the library files in your system's default library folder.

`$ make install`

You can check whether your installation was successful by running the `wx-config` command.

```
$ wx-config --list
    Default config is osx_cocoa-unicode-static-3.1
  Default config will be used for output
```

Finally, you can remove the build files. They are no longer needed and take up disk space.

`$ make clean`

If you ever wish to remove the installation, run the following command from your build folder.

`$ make uninstall`

#### Cloning the repository
If you have all dependencies installed, you should be able to easily compile the program by running:

`$ make`

The compiled executable will be placed in the `bin` folder.

`$ bin/ballpit`

## Getting Help
If you wish to report a bug or request a feature go to the project's GitHub:
https://github.com/jmaglic/ballpit/issues

## Development Team
The development is currently lead by Jasmin B. Maglic and Roy Lavendomme.
