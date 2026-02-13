## Overview

This repository contains a Zephyr-based peripheral test application for the nRF54H platform
intended for evaluating hardware features.

The application provides a collection of small, focused test modules that can be
accessed via the Zephyr shell.

Currently, the project includes:
- An NFC Type 4 Tag (T4T) test for NDEF text read/write, field sensing, and field
  presence detection
- A CRC32 test for verifying memory contents

---

## Build Environment

The application was built using:

- **nRF Connect SDK (NCS): v3.2.1**
- **nRF Connect SDK Toolchain: v3.2.1**

The application was built using the following command:

```bash
west build -p -b nrf54h20dk/nrf54h20/cpuapp .
```
---

## NFC Test Functionality

The NFC test module provides a shell command (`nfctest`) that allows interacting
with the NFC hardware in several modes:

- **Static NDEF read**
  - Emulates an NFC Type 4 Tag with a predefined NDEF Text record
  - Waits for an external reader (e.g. a smartphone) to read the tag

- **NDEF write reception**
  - Emulates a writable NFC tag
  - Waits for an external device to write an NDEF Text record
  - Parses and prints the received text payload

- **Field sensing control**
  - Enables or disables NFC field sensing

- **Field presence detection**
  - Detects NFC field presence and measures the associated field frequency

Internally, the implementation:
- Uses Zephyr NFC T4T libraries for tag emulation
- Builds and parses NDEF Text records
- Synchronizes NFC events using mutexes and condition variables
- Supports configurable timeouts for NFC operations

---

## Usage

The NFC test is controlled via the Zephyr shell using the `nfctest` command.

### Command syntax

`nfctest <mode> [arguments]`

Parameters:
- `mode` – Test mode selector (see table below)
- `text` – NDEF text payload (mode 1)
- `timeout_ms` – timeout in milliseconds
- `submode` – NFC field sensing control (`1` = ON, `2` = OFF)

### Modes

| Mode | Description |
|------|------------|
| `1 <text> [timeout_ms]` | Emulate a tag with a static text payload and wait for a read |
| `2 [timeout_ms]` | Emulate a writable tag and wait for a write |
| `3 <submode>` | NFC field sensing control (`1` = ON, `2` = OFF) |
| `4 [timeout_ms]` | Perform an NFC field presence test (timeout used for test practicality) |

A standard NFC-capable smartphone can be used as the reader or writer.

For the field presence test (mode 4), a timeout parameter is required to allow
practical testing with a smartphone. In a production use case, the intended
behavior would operate without timeouts.

---

## Notes

- Only simple **NDEF Text records** are supported.
- The implementation assumes a single NFC interaction at a time.
- Logging and error handling are primarily intended for debugging.

---

## CRC32 Test Functionality

The CRC32 test provides a simple mechanism for computing and verifying CRC32
checksums.

### Implementation Details

The CRC32 implementation is derived from the
*bzip2/libbzip2* reference implementation and follows the same algorithm and
initialization/finalization steps. The original work is:

- **bzip2/libbzip2 `crc32.c`**
- Copyright (C) 1996–2010 Julian R. Seward
- The original license text for the derived code is included in the `LICENSES.txt`
file.

The code has been adapted for use in a Zephyr-based environment and operates on
32-bit words rather than byte streams.

Key characteristics:
- CRC initialization and finalization match the bzip2 reference
- Input data is processed word-by-word, in reverse order
- Each word is processed MSB-first to remain compatible with the original
  algorithm

### Test Behavior

The test supports two operating modes:
- **Calculation only**: computes and prints the CRC32 value for a given memory
  range
- **Verification**: computes the CRC32 value and compares it against a reference
  CRC stored immediately after the data region

The test validates input parameters such as word alignment and length before
processing.

### Usage

The CRC32 test is accessed via the Zephyr shell using the `crc32` command.

### Command syntax

`crc32 <address> <words> <mode>`

Parameters:
- `address` – Start address (must be 32-bit aligned)
- `words` – Number of 32-bit words to process
- `mode` – Operating mode (see table below)

### Modes

| Mode | Description |
|------|------------|
| `0` | Calculate and print the CRC32 for the specified address and word count |
| `1` | Verify the CRC32 against a reference value stored immediately after the data |

In verification mode, the reference CRC is expected to be located immediately
after the data region.

---

## Notes

- No memory protection checks are performed beyond basic alignment and length
  validation.
- The implementation assumes the memory region is accessible and stable during
  the operation.

