# Waveshare 1.69-inch LCD driver

Application-owned driver for the 240x280 Waveshare IPS TFT module with an
ST7789V2 controller. The current application uses it in 280x240 landscape mode.

The initialization values and 20-pixel controller-RAM offset are derived from
the known-good `Waveshare169_driver` implementation. This version adds:

- checked HAL return values;
- explicit driver status and diagnostic counters;
- bounded DMA waits and separate completion/error outcomes;
- argument and rectangle validation;
- ThreadX synchronization;
- partial-rectangle transfers;
- no private framebuffer or heap allocation.

The application retains its already-working landscape settings: MADCTL `0x60`
and COLMOD `0x55`. The standalone reference uses MADCTL `0x70` and COLMOD
`0x05`; those values were not copied blindly because they change scan-direction
and interface-format bits.

`Core/Src/unified.c` is a compatibility adapter. New display-specific code
should call this library directly. CubeMX-generated files do not own this
folder and will not overwrite it.

Non-destructive geometry and boundary tests live under `App/Tests`. They are
included in the existing `TEST` console command and do not access SPI or alter
the display contents.
