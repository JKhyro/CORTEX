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

## Native runtime slices

The first native C runtime slices are the imported catalog mapper for `CORTEX #19` and the character/ARA lifecycle core for `CORTEX #21`.

```powershell
cmake -S . -B "$env:LOCALAPPDATA\CORTEX\runtime-build"
cmake --build "$env:LOCALAPPDATA\CORTEX\runtime-build" --config Debug
ctest --test-dir "$env:LOCALAPPDATA\CORTEX\runtime-build" -C Debug --output-on-failure
```

The current native entry points are [include/cortex/imported_catalog_mapper.h](include/cortex/imported_catalog_mapper.h) and [include/cortex/character_runtime.h](include/cortex/character_runtime.h). They expose the mapper ABI, character/runtime ABI, lifecycle state/actions, stable outcome/error enums, governed imported-helper binding, and subagent capacity guards.

## Current specification set

- [docs/canonical-entity-model.md](docs/canonical-entity-model.md): canonical classes, ownership rules, import boundaries, exports, and provenance handling for `CORTEX #6`
- [docs/lifecycle-state-machine.md](docs/lifecycle-state-machine.md): character, component, and subagent lifecycle states and control verbs for `CORTEX #5`
- [docs/prism-integration-contract.md](docs/prism-integration-contract.md): bounded `PRISM -> CORTEX` contract for personality matrices, overlays, stylization, and governance envelopes for `CORTEX #4`
- [docs/ascent-integration-contract.md](docs/ascent-integration-contract.md): bounded `ASCENT -> CORTEX` contract for progression state, rewards, penalties, and achievement inputs for `CORTEX #11`
- [docs/runtime-topology.md](docs/runtime-topology.md): native-first runtime topology for child programs, host roles, and explicit bridge boundaries for `CORTEX #7`
- [docs/packaging-distribution-model.md](docs/packaging-distribution-model.md): native-first packaging, distribution, and artifact compatibility model for `CORTEX #8`
- [docs/imported-catalog-mapping.md](docs/imported-catalog-mapping.md): imported helper/subagent catalog mapping contract, template and overlay contracts, lifecycle ownership, error behavior, compatibility policy, and example mappings for `CORTEX #9`
- [docs/external-integration-contract.md](docs/external-integration-contract.md): governed export and registration contract for `VECTOR`, `NEXUS`, and external execution targets for `CORTEX #3`
- [docs/character-management-ui-contract.md](docs/character-management-ui-contract.md): thin-host creation and management surface contract for `CORTEX #10`
- [prototypes/cortex-control-surface.html](prototypes/cortex-control-surface.html): visual prototype for the control surface, current execution wave, and progress visibility surface

## Current execution status

- `CORTEX #3`: Done
- `CORTEX #9`: Done
- `CORTEX #19`: Done
- `CORTEX #21`: Done
- `CORTEX #24`: Done
- `CORTEX #2`: Done

## Functional readiness

CORTEX is roughly 38% complete toward a usable runnable product. The contract set, static progress prototype, native imported catalog mapper, native character/ARA lifecycle core, and first durable runtime persistence slice are in place with executable tests. Shell integration, real external catalog feed wiring, installable packaging, and an operator-ready host surface still remain.
