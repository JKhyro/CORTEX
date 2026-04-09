# CORTEX And PRISM Integration Contract

Issue alignment: `CORTEX #4`

## Purpose

Define the bounded `PRISM -> CORTEX` contract for personality matrix inputs, overlays, stylization rules, and governance envelopes.

`PRISM` may shape how a governed character presents or weights behavior, but it does not gain character-construction authority, lifecycle authority, or subagent-governance authority.

This contract assumes the entity model in [`canonical-entity-model.md`](./canonical-entity-model.md), the lifecycle rules in [`lifecycle-state-machine.md`](./lifecycle-state-machine.md), and the ARA doctrine in [`character-ara-doctrine.md`](./character-ara-doctrine.md).

## Interface contract

The native CORTEX core must accept only four PRISM-origin payload families:

- `PrismPersonalityMatrix`: weighted trait or style axes that influence character behavior without redefining entity ownership
- `PrismCharacterOverlay`: bounded overlays that modify presentation, stylistic posture, or constrained governance hints
- `PrismStylizationProfile`: formatting, tone, and output-shaping rules that remain subordinate to CORTEX safety, lifecycle, and export policy
- `PrismGovernanceEnvelope`: advisory governance inputs such as role emphasis, behavioral constraints, or moderation bias that CORTEX may accept, reject, or narrow

PRISM-origin payloads must be accepted only through a CORTEX-owned binding action. They are never self-authorizing.

## Expected inputs and outputs

Inputs from PRISM into CORTEX:

- a target `character_id`
- optional `component_id` targets when a payload is component-scoped
- payload provenance including source profile, profile version, and compatibility version
- matrix, overlay, stylization, or governance fields already normalized by the PRISM boundary layer
- operator intent flags when the shell is attaching, replacing, or removing a PRISM-origin binding

Outputs from CORTEX:

- accepted `personality_binding_ref` records on the target character
- accepted `overlay_ref` records on the target character or component
- normalized read-only summaries for shell presentation
- validation results that say whether the payload was accepted, narrowed, rejected, or quarantined
- stable native error families when authority or compatibility rules are violated

## Binding boundaries

PRISM may provide:

- personality weighting data for a character or component
- overlay stacks that affect presentation or bounded behavior emphasis
- stylization rules for generated output shape, tone, and vocabulary bias
- governance hints that propose constraints or emphasis without becoming lifecycle verbs

PRISM may not provide:

- character creation commands
- owner or curator reassignment
- ARA slot allocation or reorder authority
- direct lifecycle transitions such as `activate`, `suspend`, `archive`, or `despawn`
- subagent spawn authority
- persistent identity promotion
- export-surface authorization for downstream repos or external agents

## Native binding model

### Character-level bindings

- One `PrismPersonalityMatrix` may be attached as the default matrix for a character.
- Multiple overlays may be staged, but CORTEX must expose the resolved overlay order explicitly.
- One active stylization profile may be resolved per character output surface unless a stricter export policy narrows it further.

### Component-level bindings

- Component-scoped PRISM bindings are allowed only when the target `component_id` belongs to the target `character_id`.
- A component binding may bias tone, role expression, or reasoning emphasis for that component.
- Component bindings may not change the component owner, lifecycle state, or subagent ceiling.

### Governance envelopes

- Governance envelopes remain advisory until accepted by CORTEX.
- CORTEX may narrow or discard a governance envelope when it conflicts with ownership doctrine, lifecycle policy, or export safety.
- Accepted governance envelopes should be exposed as resolved constraints, not as raw PRISM authority.

## Required fields by payload family

### `PrismPersonalityMatrix`

- `matrix_id`
- `compatibility_version`
- `target_character_id`
- `target_component_ids[]`
- `trait_axes[]`
- `weighting_policy`
- `provenance_ref`

### `PrismCharacterOverlay`

- `overlay_id`
- `compatibility_version`
- `target_character_id`
- `target_component_ids[]`
- `overlay_class`
- `overlay_priority`
- `bounded_effects[]`
- `provenance_ref`

### `PrismStylizationProfile`

- `style_profile_id`
- `compatibility_version`
- `target_character_id`
- `surface_scope`
- `tone_rules[]`
- `format_rules[]`
- `vocabulary_bias[]`
- `provenance_ref`

### `PrismGovernanceEnvelope`

- `governance_envelope_id`
- `compatibility_version`
- `target_character_id`
- `target_component_ids[]`
- `constraint_hints[]`
- `role_emphasis[]`
- `moderation_bias[]`
- `provenance_ref`

## Validation rules

- A PRISM-origin payload is invalid if the target character does not exist.
- A component-scoped payload is invalid if any referenced `component_id` is not part of the target character.
- A personality matrix is invalid if it attempts to redefine owner, curator, lifecycle, or export fields.
- An overlay is invalid if it attempts to replace canonical labels, entity classes, or lifecycle states.
- A stylization profile is invalid if it requests output behavior that violates native export or governance policy.
- A governance envelope is invalid if it tries to introduce direct lifecycle verbs or subagent spawn authority.
- Unknown compatibility versions must be rejected or quarantined; they must not be silently applied.
- PRISM-origin payloads may be attached in `draft`, `validated`, `ready`, and `active` states, but attachment alone may not force a lifecycle transition.

## Error behavior

Stable native error families for this contract:

- `CORTEX_ERR_PRISM_VERSION_MISMATCH`
- `CORTEX_ERR_PRISM_UNKNOWN_TARGET_CHARACTER`
- `CORTEX_ERR_PRISM_UNKNOWN_COMPONENT_TARGET`
- `CORTEX_ERR_PRISM_OVERLAY_SCOPE_VIOLATION`
- `CORTEX_ERR_PRISM_AUTHORITY_VIOLATION`
- `CORTEX_ERR_PRISM_STYLIZATION_POLICY_CONFLICT`
- `CORTEX_ERR_PRISM_GOVERNANCE_ENVELOPE_CONFLICT`

Each failure should report:

- the failing payload family and payload id
- the failing target character or component id
- the violated rule id
- provenance metadata
- whether the payload may be retried after narrowing

## Compatibility and versioning notes

- PRISM payload families are additive by default.
- Existing accepted field meanings must not be reassigned across versions.
- The stable C ABI should expose explicit descriptor versions for PRISM-origin bindings and validation results.
- Unknown optional fields may be preserved in inert provenance storage, but they must not affect resolved native behavior until the core supports them.
- Downstream shells should render only the normalized CORTEX view of accepted PRISM bindings, not the raw PRISM payload as an authority surface.

## Example usage

```text
1. The operator selects an existing Character in CORTEX.
2. PRISM provides one normalized personality matrix, one overlay, and one stylization profile for that character.
3. CORTEX validates that all targeted component ids belong to the selected character.
4. CORTEX accepts the matrix and stylization profile, but narrows the governance envelope because one hint conflicts with lifecycle policy.
5. The shell shows the resolved PRISM bindings as read-only accepted inputs.
6. The character stays in its current lifecycle state until a separate native lifecycle action is taken.
```
