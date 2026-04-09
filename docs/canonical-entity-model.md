# CORTEX Canonical Entity Model

Issue alignment: `CORTEX #6`

## Purpose

Define the canonical CORTEX entity model for persistent identities, governed characters, ARA components, subagent instances, imported definitions, overlays, and export definitions.

The model is native-first. Avalonia and C# may present or transport the model, but they do not redefine ownership, class boundaries, or lifecycle authority.

## Interface contract

The CORTEX core must expose one authoritative class registry with these entity families:

- `Symbiote`: persistent KHYRON-owned identity record for a durable agent persona or operator-facing character owner
- `Curator`: persistent KHYRON-owned governance identity that may author, supervise, or constrain character behavior without becoming the character itself
- `Character`: the singular CORTEX-governed operational entity formed from an Adaptive Response Array (ARA)
- `AraComponent`: one bound slot inside a character's ARA with an explicit role, owner, and subagent ceiling
- `SubagentInstance`: a spawned or staged helper owned by exactly one `AraComponent`
- `BuiltInSubagentDefinition`: a native CORTEX-provided subagent template that can be instantiated by eligible components
- `ImportedHelperSubagentDefinition`: a non-persistent imported helper template that stays definition-only until instantiated by a component
- `ImportedArrayTemplate`: an imported ARA pattern that can seed character construction without becoming a top-level identity
- `ImportedCharacterOverlay`: an imported overlay that modifies presentation or constrained behavior of a character without replacing canonical ownership
- `ExternalAgentExportDefinition`: a governed export contract that presents approved character or component surfaces to downstream consumers such as `VECTOR`

## Expected inputs and outputs

Inputs to the entity model:

- persistent identity records from KHYRON-owned stores
- normalized imported definitions from the verified `SYMBIOSIS-core` import pipeline
- native CORTEX construction commands that bind templates, overlays, and definitions into characters
- bounded `PRISM` stylization data and bounded `ASCENT` progression data after they are accepted by CORTEX contracts

Outputs from the entity model:

- validated native records for characters, components, and subagent instances
- exportable views for downstream consumers
- stable identifiers and provenance metadata for lifecycle, UI, packaging, and interop layers
- validation errors when ownership, promotion, or import rules are violated

## Canonical layers

### Persistent identity layer

- `Symbiote` and `Curator` are the only first-class persistent identity classes in the current model.
- Imported definitions do not become `Symbiote` or `Curator` records by default.
- Persistent identity ownership remains KHYRON-owned even when imported definitions are loaded or mirrored through `SYMBIOSIS`.

### Governed runtime layer

- `Character` is the canonical runtime unit.
- A `Character` must reference at least one persistent owner record and one or more bound `AraComponent` records.
- `AraComponent` is the ownership boundary for subagent governance.
- Each `AraComponent` may stage or spawn up to `12` `SubagentInstance` records.

### Imported definition layer

- Imported definitions remain definition objects unless CORTEX explicitly binds or instantiates them.
- Imported helpers may map into helper slots only.
- Imported array templates may seed composition only.
- Imported character overlays may modify presentation or bounded governance behavior only.
- Imported provenance aliases stay attached to the imported definition, but CORTEX canonical labels remain the default presentation form.
- `PRISM` remains the stylization authority; imported style hints do not override CORTEX ownership or lifecycle.

### Export layer

- `ExternalAgentExportDefinition` describes what a downstream consumer may see or control.
- Exports may expose approved character views, component views, or operator-safe control verbs.
- Exports do not transfer lifecycle authority out of CORTEX.

## Core fields by class

### `Symbiote`

- `symbiote_id`
- `canonical_label`
- `identity_origin = khyron`
- `governance_profile_ref`
- `status`

### `Curator`

- `curator_id`
- `canonical_label`
- `identity_origin = khyron`
- `governance_scope`
- `status`

### `Character`

- `character_id`
- `canonical_label`
- `owner_identity_ref`
- `governor_identity_refs[]`
- `array_shape`
- `component_refs[]`
- `overlay_refs[]`
- `progression_binding_refs[]`
- `personality_binding_refs[]`
- `lifecycle_state`

### `AraComponent`

- `component_id`
- `character_id`
- `slot_index`
- `role`
- `owner_identity_ref`
- `governance_mode`
- `subagent_limit = 12`
- `subagent_instance_refs[]`
- `lifecycle_state`

### `SubagentInstance`

- `subagent_instance_id`
- `component_id`
- `definition_ref`
- `origin = builtin | imported`
- `spawn_mode`
- `lifecycle_state`

### Imported definition classes

- `definition_id`
- `canonical_label`
- `provenance_aliases[]`
- `source_manifest_ref`
- `promotion_policy`
- `compatibility_version`

### `ExternalAgentExportDefinition`

- `export_id`
- `source_entity_ref`
- `exposed_surface`
- `allowed_queries[]`
- `allowed_control_verbs[]`
- `compatibility_version`

## Ownership and promotion rules

- Every `Character` must have exactly one primary persistent owner identity.
- `Curator` records may supervise without replacing the primary owner.
- `AraComponent` ownership may differ from character ownership only when the character-level governance profile allows delegated ownership.
- `SubagentInstance` ownership always flows through the owning `AraComponent`; subagents never become top-level identity records.
- Imported helpers, templates, overlays, and export definitions remain non-persistent until an explicit native CORTEX promotion action records them into a persistent class.
- Promotion into a persistent class requires explicit operator approval and a native audit event.

## Validation rules

- Two-component and three-component ARAs are first-class array shapes.
- Larger arrays are allowed only when a registered template or construction rule justifies them.
- A `Character` is invalid if it has zero components, duplicate slot indexes, or unresolved owner references.
- An `AraComponent` is invalid if its role is undefined, its owner is unresolved, or its subagent count exceeds `12`.
- A `SubagentInstance` is invalid if it is not backed by a compatible built-in or imported definition.
- An `ImportedCharacterOverlay` is invalid if it attempts to replace canonical ownership, lifecycle, or top-level identity labels.
- An `ExternalAgentExportDefinition` is invalid if it exposes destructive control verbs outside the governed export policy.

## Error behavior

Native validation and interop layers should use stable error families:

- `CORTEX_ERR_INVALID_OWNER`
- `CORTEX_ERR_INVALID_COMPONENT_ROLE`
- `CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED`
- `CORTEX_ERR_UNRESOLVED_IMPORT_DEFINITION`
- `CORTEX_ERR_ILLEGAL_PROMOTION`
- `CORTEX_ERR_OVERLAY_AUTHORITY_VIOLATION`
- `CORTEX_ERR_EXPORT_SCOPE_VIOLATION`

Errors must report:

- failing entity id
- failing rule id
- source provenance when imports are involved
- suggested remediation when the failure is operator-fixable

## Compatibility and versioning notes

- The entity-class registry is additive by default.
- Existing class meanings must not be reassigned across versions.
- Imported definition manifests should carry a compatibility version that can be mapped by the native core before instantiation.
- The stable C ABI should expose explicit version numbers for entity descriptors and validation results.
- Unknown imported classes must be rejected or staged as inert definitions; they must not be silently promoted into persistent identity classes.

## Example usage

```text
1. Load a persistent Symbiote owner and a persistent Curator governor.
2. Instantiate a Character with array_shape=two_component.
3. Bind component slot 0 as role=active and component slot 1 as role=memory.
4. Attach one imported helper definition to slot 0 as a staged SubagentInstance.
5. Attach one imported overlay with provenance aliases preserved.
6. Expose a read-only ExternalAgentExportDefinition for VECTOR.
7. Validate the Character; reject activation if owner, component, or import rules fail.
```
