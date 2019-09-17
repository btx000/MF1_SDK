MF1_SDK_PlatformIO_Project
==========

SDK for Sipeed MF1 AI module. This project modify from [https://github.com/sipeed/MF1_SDK](https://github.com/sipeed/MF1_SDK)
If you are using Linux, choosing the original project is better. 

## Pre Built Firmware

[http://dl.sipeed.com](http://dl.sipeed.com/MAIX/SDK/MF1_SDK_Prebuild/dev)

## Get source code

* Clone by https link

```
git clone https://github.com/sipeed/MF1_SDK.git
```

* Then get submodules

```
git submodule update --recursive --init
```

It will regiter and clone all submodules, if you don't want to register all submodules, cause some modules is unnecessary, just execute
```
git submodule update --init
# or 
git submodule update --init path_to_submodule
# or
git submodule update --init --recursive path_to_submodule
```

## Install dependencies

### Install Python3

Download python3 from [here](https://www.python.org/ftp/python/3.7.4/python-3.7.4-amd64.exe)

Use defult install and add python to `PATH`.

### Install Virtual Environment

Pip install virtualenv

```
pip install virtualenv
```

More info about pio virtualenv. [https://docs.platformio.org/en/latest/installation.html?highlight=virtualenv#virtual-environment](https://docs.platformio.org/en/latest/installation.html?highlight=virtualenv#virtual-environment)

### Install VScode and PlatformIO

There is a install guide.[http://blog.sipeed.com/p/622.html](http://blog.sipeed.com/p/622.html)

## Configure project

Open this project directory with vscode.

You can modify some compile and download options by modifying `platformio.ini`, modify `include/global_config.h` to configure development board macro definitions.

* platformio.ini

```ini
[env:sipeed-mf1]
platform = kendryte210             
platform_packages = framework-kendryte-standalone-sdk @ https://github.com/kendryte/kendryte-standalone-sdk.git  ;This option keeps sdk up to date.
framework = kendryte-standalone-sdk
board = sipeed-MF1                
monitor_speed = 115200
upload_port = COM18       ;Choose the correct download port
build_flags = -l_face_dual_hor -L./src/face_lib/lib   ;Select the corresponding function library according to the hardware device.
extra_scripts = 
  pre:pre_build.py                                    ;Custom build script

```

If you want to add custom compilation steps, you can modify `pre_build.py`. More usage can refer to the [documentation](https://docs.platformio.org/en/latest/projectconf/advanced_scripting.html)

## Build

Compiling with platformio is very simple, you just click the compile button

## Flash (Burn) to board

Flash is simple as build, just click upload button.

## Others

See [https://docs.platformio.org/en/latest/](https://docs.platformio.org/en/latest/)






