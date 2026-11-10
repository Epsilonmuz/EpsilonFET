# Epsilon FET - engine measurements

Measured by `tests/measure_engine.py` on the circuit engine at 176.4 kHz (4x 44.1 kHz). Signals are 1 kHz sines unless stated; levels are dBFS at the plugin input, with -18 dBFS peak = +4 dBu. These are measurements of the model against the published 1176 specifications, not of a hardware unit.

## Static compression (gain reduction in dB, Input 0 dB, attack 3, release 5)

| Input level | 4:1 | 8:1 | 12:1 | 20:1 | All |
|---|---|---|---|---|---|
| -36 dBFS | 0.0 | 0.0 | 0.0 | 0.0 | -0.9 |
| -30 dBFS | -0.1 | -0.0 | 0.0 | 0.0 | -2.3 |
| -24 dBFS | -1.3 | -1.1 | -0.2 | 0.0 | -6.1 |
| -18 dBFS | -5.9 | -4.7 | -3.5 | -2.3 | -11.0 |
| -12 dBFS | -10.6 | -10.3 | -9.0 | -7.5 | -16.5 |
| -6 dBFS | -15.0 | -15.7 | -14.6 | -13.2 | -21.6 |
| 0 dBFS | -19.1 | -20.8 | -19.9 | -18.8 | -26.3 |

Slope above the knee (output dB per input dB, from +6 to +18 dB above threshold):

- 4:1: measured 0.221 dB/dB = 4.5:1
- 8:1: measured 0.082 dB/dB = 12.2:1
- 12:1: measured 0.066 dB/dB = 15.3:1
- 20:1: measured 0.042 dB/dB = 23.9:1

## Timing (step -40 to -12 dBFS and back, ratio 4:1)

| Knob | Attack, 63 % of final GR | Release, 63 % recovery |
|---|---|---|
| 7 | 102 us | 52 ms |
| 6 | 108 us | 87 ms |
| 5 | 125 us | 146 ms |
| 4 | 147 us | 244 ms |
| 3 | 198 us | 409 ms |
| 2 | 289 us | 680 ms |
| 1 | 760 us | 1088 ms |

Published 1176 ranges: attack 20 us - 800 us, release 50 ms - 1.1 s. At 1 kHz the fastest attack settings are limited by the waveform itself (the detector cannot charge before the sine rises); the detector time constant at knob 7 is 20 us (90 ohm x 0.22 uF).

## Distortion (THD, harmonics 2-9)

| Condition | THD | H2 | H3 |
|---|---|---|---|
| 1 kHz -18 dBFS, compressor off | 0.10 % | -60 dB | -81 dB |
| 1 kHz -6 dBFS, compressor off | 0.44 % | -48 dB | -54 dB |
| 1 kHz -18 dBFS, 4:1, ~4 dB GR | 0.25 % | -62 dB | -54 dB |
| 1 kHz -6 dBFS, 20:1, ~13 dB GR | 0.34 % | -57 dB | -52 dB |
| 1 kHz -6 dBFS, all buttons | 0.27 % | -70 dB | -53 dB |
| 100 Hz -10 dBFS, 4:1, attack 7 / release 7 | 2.67 % | -63 dB | -33 dB |
| 100 Hz -10 dBFS, 4:1, attack 3 / release 3 | 0.34 % | -63 dB | -51 dB |
| 50 Hz -10 dBFS, 4:1, attack 7 / release 7 | 4.66 % | -65 dB | -28 dB |

Fast release on bass ripples the FET gate within each cycle: that is the hardware's known low-frequency grit, and it disappears with slower settings.

## Sample-rate invariance (GR at -10 dBFS, 4:1)

- 88.2 kHz internal: -12.09 dB
- 96 kHz internal: -12.09 dB
- 176.4 kHz internal: -12.08 dB
- 192 kHz internal: -12.08 dB
- 384 kHz internal: -12.08 dB
