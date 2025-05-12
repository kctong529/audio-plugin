# Audio Plugin

This repository contains assignments and projects from an Audio Programming course, created by two developers exploring audio plugin development for use in Digital Audio Workstations (DAWs).

## Cloning the Repository

Make sure to clone the repository with its submodules:

```
git clone --recurse-submodules git@github.com:kctong529/audio-plugin.git
```

### Assignment 1: Ring Modulator

A ring modulator is an amplitude-modulating signal processing effect that multiplies a carrier signal with a modulator signal. In this assignment, the input audio signal `x` is multiplied by a cosine carrier signal `c = cos(ωn)` to produce the output signal `y`. The plugin includes at least one adjustable parameter to control the **modulation frequency** (in Hz), allowing users to shape the modulation characteristics in real-time.

## Build the example project

To build the example project:

1. Open the project in **Visual Studio Code**.
2. Delete the existing `build` directory if one exists — this helps prevent build conflicts.
3. Build the project using your preferred method in VSCode.

