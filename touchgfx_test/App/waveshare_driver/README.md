# Waveshare 1.69-inch LCD driver

Application-owned driver for the 240x280 Waveshare IPS TFT module with an
ST7789V2 controller. The current application uses it in 280x240 landscape mode.

The initialization values and 20-pixel controller-RAM offset are derived from
the known-good `waveshare_driver` implementation. This version adds:

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

The driver contains the board's SPI2 and display-pin setup as well as the
panel operations. CubeMX-generated files do not own this folder and will not
overwrite it.

The hardware-independent display geometry (`ws169_geometry`) lives in `App/logic/` and is covered
by unit tests in `Tests/test_ws169_geometry.c` (run `scripts/unit-test.sh`).
