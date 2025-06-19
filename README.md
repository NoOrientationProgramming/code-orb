
<h2 id="codeorb-start" style="display:none;"></h2>

![GitHub](https://img.shields.io/github/license/NoOrientationProgramming/code-orb?style=plastic&color=orange)
[![GitHub Release](https://img.shields.io/github/v/release/NoOrientationProgramming/code-orb?color=orange&style=plastic)](https://github.com/NoOrientationProgramming/code-orb/releases)

![Windows](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/windows.yml?style=plastic&logo=github&label=Windows)
![Linux](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/linux.yml?style=plastic&logo=linux&logoColor=white&label=Linux)
![MacOS](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/macos.yml?style=plastic&logo=apple&label=MacOS)
![FreeBSD](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/freebsd.yml?style=plastic&logo=freebsd&label=FreeBSD)
![ARM, RISC-V & MinGW](https://img.shields.io/github/actions/workflow/status/NoOrientationProgramming/code-orb/cross.yml?style=plastic&logo=gnu&label=ARM%2C%20RISC-V%20%26%20MinGW)

[![Discord](https://img.shields.io/discord/960639692213190719?style=plastic&color=purple&logo=discord)](https://discord.gg/FBVKJTaY)
[![Twitch Status](https://img.shields.io/twitch/status/Naegolus?label=twitch.tv%2FNaegolus&logo=Twitch&logoColor=%2300ff00&style=plastic&color=purple)](https://twitch.tv/Naegolus)

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/res/codeorb.jpg" style="width: 700px; max-width:100%"/>
  </kbd>
</p>

## The Microcontroller Debugger

When working with small targets, simple log outputs are often the only feedback available.
With [CodeOrb](https://github.com/NoOrientationProgramming/code-orb#codeorb-start) on the PC and the
[SystemCore](https://github.com/NoOrientationProgramming/SystemCore#processing-start) on the target,
we have two additional channels: a task viewer and a command interface.
The task viewer provides a detailed insight into the entire system, whereas the command interface gives full control over the microcontroller.

CodeOrb is essentially a multiplexer service running on the PC that transmits and receives these three channels of information via UART to and from the microcontroller.
The channels can then be viewed on the PC or over the network using a Telnet client such as PuTTY.

## Features

- Full control over target
- Crystal-clear insight into the system
- Through three dedicated channels
  - **Process Tree** on TCP port 3000
  - **Log** on TCP port 3002
  - **Interactive Command Interface** on TCP port 3004

## How To Use

### Topology

This repository provides `CodeOrb` the microcontroller debugger highlighted in orange. Check out the [example for STM32](https://github.com/NoOrientationProgramming/hello-world-stm32) as well!

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/system/topology.svg" style="width: 300px; max-width:100%"/>
  </kbd>
</p>

### User Interface

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/screenshots/Screenshot%20from%202025-05-26%2022-25-18.png" style="width: 700px; max-width:100%"/>
  </kbd>
</p>

## How To Build

### Requirements

You will need [meson](https://mesonbuild.com/) and [ninja](https://ninja-build.org/) for the build.
Check the instructions for your OS on how to install these tools.

### Steps

Clone repo
```
git clone https://github.com/NoOrientationProgramming/code-orb.git --recursive
```

Enter the directory
```
cd code-orb
```

Setup build directory
```
meson setup build-native
```

Build the application
```
ninja -C build-native
```

## Start CodeOrb

On Windows
```
.\build-native\CodeOrb.exe -d COM1
```

On UNIX systems
```
./build-native/codeorb -d /dev/ttyACM0
```

The output should look like this
<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/screenshots/Screenshot%20from%202025-06-19%2022-29-20.png" style="width: 700px; max-width:100%"/>
  </kbd>
</p>
