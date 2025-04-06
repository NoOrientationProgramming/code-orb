![GitHub](https://img.shields.io/github/license/NoOrientationProgramming/gw-dbg-swt?style=plastic)

Gateway for SingleWireTransfering()

## What You Get

- Full control over your target
- Using three channels
  - Process Tree
  - Log
  - Command Interface

### Process Tree

TODO: Screenshot

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
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/gw-dbg-swt/main/doc/system/stm32-uart_3.svg" style="width: 400px; max-width:100%"/>
  </kbd>
</p>

### Overview of the Gateway

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/gw-dbg-swt/main/doc/system/overview-gw.svg" style="width: 300px; max-width:100%"/>
  </kbd>
</p>

### Internal Structure of the Single Wire Protocol Controller

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/gw-dbg-swt/main/doc/system/structure-dbg-internal.svg" style="width: 300px; max-width:100%"/>
  </kbd>
</p>

### Communication Example between Controller and Target

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/gw-dbg-swt/main/doc/system/protocol/B1_swt-ctrl-to-target_cmd_cmd-resp.svg" style="width: 800px; max-width:100%"/>
  </kbd>
</p>
