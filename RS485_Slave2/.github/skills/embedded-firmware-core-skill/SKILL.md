---
name: embedded-firmware-core-skill
description: "Universal embedded firmware core skill for production C code. Enforces think-before-coding, simplicity-first design, surgical changes, goal-driven execution, deterministic behavior, and verifiable quality gates."
---

# Embedded Firmware Code Skill

## Purpose
This skill is the core standard for writing, modifying, and reviewing production embedded firmware.

It defines mandatory behavior for:
- New feature implementation
- Bug fixing and regression repair
- Refactors and module restructuring
- Code review and quality gate checks

This is a core skill, not a project-specific rulebook.
Project details must be placed in separate overlay skills.

## Four Core Principles
1. Think Before Coding
- Clarify goals, assumptions, constraints, and unknowns before touching code.
- Identify trade-offs up front (correctness, complexity, timing, memory, delivery risk).
- Resolve ambiguity first; do not encode guesses into implementation.

2. Simplicity First
- Prefer the smallest complete solution.
- Avoid speculative abstractions and over-generalization.
- Keep interfaces narrow and control flow explicit.

3. Surgical Changes
- Make minimal and orthogonal edits tied directly to the target goal.
- Avoid unrelated edits, style drift, and broad collateral changes.
- Keep diffs easy to review, validate, and rollback.

4. Goal-Driven Execution
- Define measurable success criteria before implementation.
- Verify behavior with build checks and runtime evidence.
- Consider work complete only when success criteria are demonstrated.

## Rule Priority
If rules conflict, apply in this order:
1. Safety and correctness
2. Determinism and recoverability
3. Maintainability and readability
4. Performance and resource efficiency
5. Local coding convenience

## Mandatory C Style and Naming
- Use fixed-width integer types for firmware logic.
- Use 4 spaces for indentation; do not use tabs.
- Place braces on separate lines.
- Use one statement per line.
- Use snake_case for function names.
- Use s_ prefix for static global/module variables.
- Use ALL_CAPS_WITH_UNDERSCORES for macros.
- Keep naming and formatting consistent inside each touched region.
- Remove magic numbers from control logic, timing, retry, thresholds, and limits.

## Design and Architecture Rules
- Keep modules single-purpose and cohesive.
- Separate hardware access, protocol codec, scheduling/state control, and app orchestration.
- Keep public APIs small and stable.
- Initialize all runtime context explicitly.
- Prefer explicit state machines over hidden state behavior.
- Avoid hidden cross-module dependencies.

## Concurrency, ISR, and Timing Rules
- ISR logic must be short, bounded, and non-blocking.
- Do not place heavy computation or blocking behavior in ISR.
- Any loop that waits for events must include timeout and failure exit.
- Shared data across ISR/task/context must have explicit ownership and consistency strategy.
- State transitions must be explicit, auditable, and recoverable.

## Reliability and Safety Rules
- External interactions must use bounded timeout and bounded retry where applicable.
- Failure paths must be explicit and observable by return code, state, or counters.
- Startup and reset paths must restore a safe and deterministic runtime state.
- Recovery from offline/fault state must be intentional and testable.

## Input Validation and Diagnostics
- Treat external input as untrusted.
- Validate pointers, lengths, indexes, enums, and frame boundaries before use.
- Never trigger state-changing actions from unvalidated payloads.
- Maintain minimal diagnostics for field debugging:
  - success and failure counters
  - online and offline state
  - consecutive failure counters

## Commenting Rules (Mandatory)
- Every newly added code block must include detailed comments so first-time readers can understand intent and flow at a glance.
- Every newly added function, variable, macro, and non-trivial statement must be commented.
- Comments must describe purpose, assumptions, key branches, and failure handling where relevant.
- Every new source/header file must start with a file header block including at least:
  - @file
  - @brief
  - responsibilities and key flow summary
- Use // for inline and local logic comments.
- Keep comments precise and maintainable; avoid obvious noise comments.

## Refactor and Change Safety Gates
- For structural refactors, compile after each meaningful step.
- Keep one logical change per patch when practical.
- If a structural edit breaks build, repair symbol/include consistency first.
- Preserve external APIs unless interface changes are explicitly requested.

## Completion Criteria (Definition of Done)
A change is complete only when all are true:
- Build succeeds for the target.
- No new blocking loop without timeout is introduced.
- New logic includes required detailed comments.
- Input validation and bounds checks are present in touched risk paths.
- State transitions and failure handling are explicit.
- Verification evidence is recorded in the final report.

## Required Review Output Format
When reviewing, report in this order:
1. Critical safety and determinism findings
2. Logic and state-transition findings
3. Maintainability and style findings
4. Residual risk and missing tests

Each finding must include:
- Severity
- File and line
- Why it matters
- Minimal fix suggestion

## Forbidden Patterns
- Hidden delays or hidden constants in logic
- Infinite waits without timeout exits
- Unchecked buffer/array access
- Silent failure paths
- Signed/unsigned mixing without intent
- Large unrelated edits in one patch

## Exceptions Policy
Deviation is allowed only when all are true:
- Hardware or protocol constraints force the deviation
- Scope is local and minimal
- The reason is documented inline
- Safety and determinism are not weakened
