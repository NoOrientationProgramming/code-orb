![GitHub](https://img.shields.io/github/license/NoOrientationProgramming/code-orb?style=plastic)

In microcontroller programming, simple log outputs are usually the only feedback available.
With [Processing()](https://github.com/NoOrientationProgramming/ProcessingCore), you get two additional features: a task viewer and a command interface.
The task viewer provides a detailed insight into the entire system, whereas the command interface gives full control over the microcontroller.

## What You Get

- Full control over your target
- Using three channels
  - Process Tree
  - Log
  - Command Interface

### Process Tree

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/screenshots/Screenshot%20from%202025-04-08%2022-40-02.png" style="width: 700px; max-width:100%"/>
  </kbd>
</p>

### Log

TODO: Screenshot

### Command Interface

TODO: Screenshot

## Status

- Pre alpha

## Targets

- Linux
- Windows
- MacOSX
- FreeBSD
- Raspberry Pi

## Architecture

### Overall Debugging Structure

This repository provides the `SingleWireTransfering() Gateway` highlighted in orange. Check out the [application](https://github.com/NoOrientationProgramming/hello-world-stm32) for the microcontroller as well!

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/system/stm32-uart_3.svg" style="width: 400px; max-width:100%"/>
  </kbd>
</p>

### Overview of the Gateway

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/system/overview-gw.svg" style="width: 300px; max-width:100%"/>
  </kbd>
</p>

### Internal Structure of the Single Wire Protocol Controller

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/system/structure-dbg-internal.svg" style="width: 300px; max-width:100%"/>
  </kbd>
</p>

### Communication Example between Controller and Target

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/code-orb/main/doc/system/protocol/B1_swt-ctrl-to-target_cmd_cmd-resp.svg" style="width: 800px; max-width:100%"/>
  </kbd>
</p>
