# Audio Plugin

This repository contains assignments and projects from an Audio Programming course, created by two developers exploring audio plugin development for use in Digital Audio Workstations (DAWs).

## Cloning the Repository

Make sure to clone the repository with its submodules:

```
git clone --recurse-submodules git@github.com:kctong529/audio-plugin.git
```

### Homework 0: Ring Modulator

A ring modulator is an amplitude-modulating signal processing effect that multiplies a carrier signal with a modulator signal. In this assignment, the input audio signal `x` is multiplied by a cosine carrier signal `c = cos(ωn)` to produce the output signal `y`. The plugin includes at least one adjustable parameter to control the **modulation frequency** (in Hz), allowing users to shape the modulation characteristics in real-time.

### Homework 1: Modulated Feedback Delay

A modulated feedback delay effect is an audio processing technique that creates decaying repetitions of the input signal by feeding delayed versions of the signal back into itself. In this assignment, we implement a delay plugin where the delay time, feedback gain, and wet/dry mix are fully controllable and smoothly adjustable to avoid artifacts.

```matlab
y[n] = g_wet * ( x[n−D] + g_fb * y[n−D] ) + g_dry * x[n]
```

Additionally, the effect can be extended with a low-pass filter in the feedback path to emulate the frequency roll-off of analog tape delays, and a subtle sinusoidal modulation of the delay time to replicate the natural pitch variations caused by tape motor inconsistencies, known as wow and flutter. The result is a versatile delay effect ranging from clean digital echoes to warm, vintage-style tape delay sounds.

## Build the example project

To build the example project:

1. Open the project in **Visual Studio Code**.
2. Delete the existing `build` directory if one exists — this helps prevent build conflicts.
3. Build the project using your preferred method in VSCode.

