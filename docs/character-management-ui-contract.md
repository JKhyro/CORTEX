# CORTEX Character Creation And Management UI Contract

Issue alignment: `CORTEX #10`

## Purpose

Define the thin-host UI contract for character creation, management, and project-progress visibility without moving CORTEX authority into the shell.

This contract assumes the entity model in [`canonical-entity-model.md`](./canonical-entity-model.md), the lifecycle rules in [`lifecycle-state-machine.md`](./lifecycle-state-machine.md), the PRISM boundary in [`prism-integration-contract.md`](./prism-integration-contract.md), and the ASCENT boundary in [`ascent-integration-contract.md`](./ascent-integration-contract.md).

## Interface contract

The desktop shell should expose five coordinated surfaces:

- `Execution Wave Rail`: a project-facing panel that shows the current CORTEX work wave and highlights the active spec or implementation lane
- `Character Composer`: the creation surface for two-component and three-component ARAs
- `Component Inspector`: role, owner, subagent capacity, and import-binding inspection for the currently selected component
- `Governance Inputs`: bounded `PRISM` and `ASCENT` panels that show what can be attached to a character without transferring authority
- `Lifecycle Console`: current state, legal actions, and pending warnings for the selected character or component

## Expected inputs and outputs

Inputs:

- current character and component descriptors from the native core
- lifecycle state snapshots and legal action sets
- bounded `PRISM` personality payloads already accepted by CORTEX
- bounded `ASCENT` progression payloads already accepted by CORTEX
- execution-wave metadata for the repo lane so the interface can show current project progress

Outputs:

- character construction requests
- component bind and reorder requests
- lifecycle action requests
- staged personality and progression attachment requests
- read-only progress visualization for the CORTEX execution wave

## Creation flows

### Two-component ARA

- Step 1: choose owner identity and canonical character label
- Step 2: allocate `active` and `memory` component slots
- Step 3: bind component owners, import definitions, and overlays
- Step 4: validate the character
- Step 5: move to `ready` or fix validation errors

### Three-component ARA

- Step 1: choose owner identity and canonical character label
- Step 2: allocate `active`, `memory`, and `reasoning` component slots
- Step 3: bind owners, imports, and overlays
- Step 4: inspect per-component subagent capacity and lifecycle guards
- Step 5: validate and promote to `ready`

### Larger arrays

- Larger arrays remain allowed, but the shell should require an explicit template or operator justification instead of exposing them as the default first-run path.

## Required presentation rules

- Role, owner, provenance, and lifecycle state must be visible together for the selected component.
- The per-component subagent ceiling must be rendered as a governed limit, not as a casual suggestion.
- Imported provenance aliases may be shown in secondary text, but CORTEX canonical labels must remain the default display label.
- `PRISM` inputs should appear as stylization and governance attachments, never as lifecycle or ownership overrides.
- `PRISM` attachments should be separated into matrix, overlay, stylization, and governance-envelope views so the operator can see which layer is active.
- `ASCENT` inputs should appear as progression and reward-state attachments, never as construction authority.
- `ASCENT` attachments should be separated into achievement, progression, reward, and penalty views so the operator can see what is earned, staged, or currently gated.

## Project-progress visibility

The UI should also help an operator visually interpret CORTEX progress in the current execution wave:

- show completed lanes and the remaining queue together without pretending already-merged work still blocks the active lane
- highlight the active lane and the next unlocked lane
- map each lane to the surface it affects, such as entity model, lifecycle, integration, UI, or packaging
- show lane state as `done`, `active`, or `queued` so merged work is visually distinct from the current drafting lane
- show aggregate execution progress such as merged-lane counts or percentages derived from repo truth
- show first-wave planning progress separately from overall product-readiness progress so specification completion is not mistaken for a functional product
- keep this panel read-only so project visibility does not become a second planning authority

## Lifecycle console behavior

- Only actions legal in the current native lifecycle snapshot should be enabled.
- If a state is `degraded`, the surface should show the degraded component or dependency, the blocked actions, and the recovery path.
- Archive actions must show the live-child safeguard before they can be confirmed.

## Error behavior

The shell should display native failures without rewriting them into ambiguous UX-only language.

Required error classes:

- `validation`
- `ownership`
- `import`
- `lifecycle`
- `capacity`
- `interop`

Each error should show:

- failing entity or component
- native rule id or error family
- actionable remediation

## Compatibility and versioning notes

- The shell must treat native legal-action lists as authoritative.
- Future component roles may be added without redesigning the overall layout.
- Unknown imported definition classes may be shown as unsupported attachments, but must not be auto-bound.
- UI schema versions should be explicit where the shell stores layout preferences or snapshots.

## Example usage

```text
1. The operator opens Character Composer and chooses the three-component flow.
2. The shell allocates active, memory, and reasoning slots from native descriptors.
3. The operator attaches a PRISM stylization input and an ASCENT progression input.
4. The Component Inspector shows current owner, provenance aliases, and 3/12 subagent capacity on the active slot.
5. The Lifecycle Console exposes validate and mark_ready because those are the only native-legal actions.
6. The Execution Wave Rail shows #6, #5, #10, #4, #11, and #7 complete, #8 active, and #3 queued as the next follow-on lane.
```
