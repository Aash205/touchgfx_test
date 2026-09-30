# Host firmware logic tests

These tests run production firmware logic on the development computer before
flashing. They do not require a board or UART and do not validate electrical
or peripheral behavior.

Configure and build with the native host compiler, then run the case-by-case
report:

```sh
cmake --preset HostTests
cmake --build --preset HostTests
ctest --preset HostTests --verbose
```

The test executable returns a nonzero exit code if any case fails. The suite
covers WS169 geometry and address-window translation, including every
rotation, full-screen and single-pixel bounds, null pointers, invalid
rotations, reversed ranges, and coordinates immediately beyond the valid
display dimensions. It also tests production UART line assembly for empty
lines, CR/LF termination, a pending command, full-buffer input, and invalid
buffer metadata.
