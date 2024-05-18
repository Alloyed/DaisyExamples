# snecho

A high-level emulation of the echo/reverb effects featured in the SNES/PSX audio chips. These effects are "fixed-function", meaning every game released for these platforms had access to the same set of effects.

Both effects run at their original sample rates and 16-bit depth, resulting in a "crunchy" sound.

## Author

<!-- Insert Your Name Here -->

## Description

Two modes are supported. when the LED is off, we're in SNES mode. when the LED is on, we're in PSX mode.

In SNES mode:

- CV 1 + CV 5 - Echo delay
- CV 2 + CV 6 - Echo feedback
- CV 3 + CV 7 - Feedback filter

In PSX mode:

- no parameters yet!

In Both:

- CV 4 + CV 8 - Wet/Dry
- BTN 7 / gate 1 - reset
- BTN 8 - toggle between snes and psx mode
