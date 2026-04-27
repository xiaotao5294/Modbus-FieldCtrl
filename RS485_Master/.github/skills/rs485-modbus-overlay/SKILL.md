---
name: rs485-modbus-overlay
description: "Project overlay skill for RS485/Modbus firmware. Apply together with embedded-c-style core skill to enforce protocol sequencing, module boundaries, and project naming rules."
---

# RS485 Modbus Overlay Skill

## Purpose
This overlay extends the embedded core skill with RS485/Modbus project constraints.
Use this only for RS485/Modbus related tasks.

## Activation Cues
- RS485 transceiver direction control
- Modbus RTU frame build/parse/validation
- UART DMA/IRQ receive pipeline related to RS485
- Master/slave polling, timeout, retry, and offline logic

## Protocol and Direction Rules
- Enforce half-duplex order strictly: switch to TX, transmit, wait completion and settle time, then switch to RX.
- Default startup direction must be RX-safe unless hardware requires otherwise.
- Never parse or act on partial frames.
- Validate address, function code, length, and CRC before payload use.

## Module Boundary Rules
- Keep protocol constants and CRC helpers in protocol-specific modules.
- Keep frame encode/decode logic in a dedicated codec module.
- Keep timeout/retry/offline scheduling logic in scheduler control modules.
- Keep orchestration task modules focused on state progression, not frame bit details.

## Naming and File Rules
- Use app_config.h as unified app config naming across master/slave when feasible.
- Use rs485_ prefix for rs485/protocol/codec/scheduler modules.
- Use task_ prefix for orchestration task modules.
- Keep include guards synchronized with file names after rename.

## Reliability Rules
- Every transaction must have bounded timeout and bounded retry.
- Distinguish transport failure, timeout, CRC failure, and protocol exception paths.
- Keep online/offline transitions explicit and recoverable.

## Diagnostics Rules
- Keep counters for send success/fail, timeout, CRC fail, and exception responses.
- Keep per-slave health summary for selection and recovery decisions.

## Forbidden Patterns
- Switching TX/RX direction without explicit completion synchronization
- Duplicating protocol literals across multiple orchestration functions
- Triggering control actions before full frame validation
