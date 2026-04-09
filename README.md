# CORTEX

CORTEX is the canonical character-construction and character-management surface in the `SYNAPSE` suite.

A CORTEX character is a singular operational entity formed from an Adaptive Response Array (ARA) of bound agent components. First-class doctrine support starts with two-component and three-component ARAs, while larger arrays remain allowed where the character model justifies them.

## Core ownership

- character creation and character management interfaces
- ARA composition, component roles, and ownership rules
- lifecycle state, control actions, and component governance
- per-component subagent governance, with each component able to spawn or despawn up to 12 subagents
- the managed contracts that bind `PRISM` personality inputs and `ASCENT` progression inputs into a governed character

## Cross-project contracts

- `PRISM -> CORTEX`: personality matrix, overlays, stylization, and character-governance inputs
- `ASCENT -> CORTEX`: achievement points, progression, reward, penalty, and state inputs
- `CORTEX -> VECTOR`: managed characters and components exposed for workspace execution
- `SYMBIOSIS -> CORTEX`: extracted OpenClaw, Lobster, and Hermes functionality that defines the character itself

## Implementation direction

- Native C is the primary implementation language for the CORTEX core.
- C++ is acceptable where it materially improves the native runtime or tooling.
- Avalonia is the preferred desktop shell where a managed UI surface is required.
- C# is used only where Avalonia, interop hosting, or platform packaging makes it necessary.
- Canonical behavior stays in native code behind an explicit, stable C ABI.

## Architecture rule

The native core owns character modeling, ARA behavior, lifecycle control, subagent governance, and creation-management logic. Avalonia and C# remain thin host layers rather than a second runtime authority.

See [docs/character-ara-doctrine.md](docs/character-ara-doctrine.md) for the character model and [docs/native-c-avalonia-direction.md](docs/native-c-avalonia-direction.md) for the stack rule and packaging implications.
