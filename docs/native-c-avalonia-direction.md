# CORTEX Native C and Avalonia Direction

## Purpose

Lock the forward CORTEX stack so implementation work does not drift between a native core and a managed reimplementation.

## Stack rule

- Native C is the canonical implementation language for CORTEX runtime and domain behavior.
- Avalonia is the preferred desktop shell where a managed UI surface is required.
- C# is allowed only where Avalonia, platform bootstrap, packaging, or interop hosting requires it.
- Native C interop must be explicit and narrow. The desktop shell should call into a stable C ABI rather than re-owning CORTEX behavior.

## What belongs in Native C

- canonical entity and identity models
- agent, subagent, and array ownership logic
- lifecycle state machines and destructive-action safeguards
- role and overlay attachment rules
- external-agent registration and catalog mapping
- runtime orchestration logic that must remain authoritative across hosts

## What may belong in C#

- Avalonia application bootstrap
- desktop windowing and shell composition
- UI-specific view-model translation
- platform-specific packaging glue
- thin interop wrappers over the native ABI

## What should not happen

- C# should not become a second source of truth for lifecycle, ownership, or role logic.
- Avalonia should not force the domain model to become managed-first.
- Interop should not hide memory ownership or invent undocumented translation behavior.

## Preferred topology

1. A Native C core library owns canonical CORTEX behavior.
2. A stable C ABI exposes commands, queries, events, and memory-handling rules.
3. An Avalonia desktop host consumes that ABI through thin C# interop.
4. Additional child programs stay native unless there is a specific UI-host reason not to.

## Packaging consequences

- Native artifacts should be first-class release outputs, not implementation details hidden behind the shell.
- If a desktop shell is shipped, it should bundle or locate the matching native core explicitly.
- Platform packaging should preserve the ability to test the native layer independently of the Avalonia shell.

## GitHub lanes this direction governs

- `CORTEX #7` runtime topology for child programs
- `CORTEX #8` packaging and distribution model
- `CORTEX #5` lifecycle state machine and operator actions
- `CORTEX #6` canonical entity model
