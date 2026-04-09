# CORTEX Character And ARA Doctrine

## Canonical unit

The canonical CORTEX unit is a character: one singular operational entity formed from an Adaptive Response Array (ARA) of bound agent components.

## Minimum first-class models

- Two-component ARA: active component plus memory component
- Three-component ARA: active component plus memory component plus advanced logic and reasoning component
- Larger arrays: allowed when the character model requires them

## Component governance

- Each component may spawn or despawn up to 12 subagents.
- Subagent governance belongs to the owning component and is exposed through CORTEX lifecycle and management controls.
- Role attachment, ownership, and lifecycle are canonical CORTEX concerns, not UI-shell concerns.

## Owned surfaces

CORTEX owns:

- character construction
- character creation UI and management UI
- ARA composition and validation
- lifecycle state and control actions
- component role rules
- subagent limits and governance

## External contracts

- `PRISM` supplies personality matrix, overlay, stylization, and character-governance inputs.
- `ASCENT` supplies achievement, progression, reward, penalty, and state inputs.
- `VECTOR` consumes CORTEX-managed characters and components for workspace execution.
- `SYMBIOSIS` routes extracted framework capability into CORTEX when that capability governs the character itself.

## Native ownership boundary

Canonical behavior belongs in Native C first. C++ is allowed where it materially helps. Avalonia and C# are host and interop layers only, through an explicit C ABI.
