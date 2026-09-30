# Repository Guidelines

## Project Structure
- Firmware: STM32L496 project. Application C/C++ code is in `Core/`, `App/`, and `TouchGFX/`.
- Third-Party / Vendor: `Drivers/`, `Middlewares/`, `AZURE_RTOS/`, `USBX/`, `BlueNRG-2/`, and any `*/generated/*` paths. Do not hand-edit these.

## Critical Build & Tooling Commands
- Build Debug: `cmake --preset Debug && cmake --build --preset Debug`
- Build Release: `cmake --preset Release && cmake --build --preset Release`
- Setup environment: `scripts/setup.sh`
- Refresh the lint compile database: `scripts/gen_compile_db.sh` (run `scripts/lint.sh` afterwards)
- Refresh Language Server database: `scripts/gen_compile_commands.sh <project-dir>`

## Strict Coding & File Rules
- Style: Follow `.clang-format` (Allman braces, 4-space indent, 100-col, CRLF line endings).
- Naming: Use `PascalCase` parameters, `snake_case` local variables, and `UPPER_SNAKE_CASE` macros.
- CubeMX Rules: Only write code inside designated `USER CODE` blocks. 
- Validation: Run `scripts/format.sh fix` and `scripts/lint.sh` before staging changes.

## Testing
- Unit tests run on the host only (no board): `scripts/host-test.sh`.
- There are no on-device tests. Code that cannot be separated from hardware is not tested.
