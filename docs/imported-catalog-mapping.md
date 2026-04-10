# CORTEX Imported Catalog Mapping Contract

Issue alignment: `CORTEX #9`

## Purpose

Define how imported external subagent catalog entries map into CORTEX helper definitions, array templates, character overlays, and lifecycle ownership without promoting imported records into persistent identities by default.

This contract depends on the canonical entity model in [`canonical-entity-model.md`](./canonical-entity-model.md) and the external boundary in [`external-integration-contract.md`](./external-integration-contract.md). It narrows the follow-on mapping lane after `CORTEX #3`.

## Interface contract

The imported catalog mapper must classify each accepted catalog entry into exactly one governed CORTEX surface:

- `ImportedHelperSubagentDefinition`: a definition-only helper template that can be staged or instantiated by an eligible `AraComponent`
- `ImportedArrayTemplate`: an imported ARA pattern that can seed component layout, role defaults, and helper placement without becoming a character
- `ImportedCharacterOverlay`: a bounded presentation or behavior overlay that can attach to a `Character` without replacing CORTEX ownership, lifecycle, or canonical labels

The mapper may return a quarantine result when the catalog entry has valid provenance but cannot be safely classified yet. It must reject entries that try to become `Symbiote`, `Curator`, `Character`, or lifecycle authority records by implication.

## Expected inputs and outputs

Inputs:

- `CortexCatalogRegistration` records accepted by the external integration boundary
- imported source provenance: `source_repo`, `source_path`, `source_commit`, and optional `source_manifest_ref`
- runtime classification hints such as `runtime_class`, `host_targets[]`, `array_binding_policy`, and `overlay_ref`
- native CORTEX owner, component, and lifecycle descriptors used to validate binding eligibility

Outputs:

- normalized `ImportedHelperSubagentDefinition`, `ImportedArrayTemplate`, or `ImportedCharacterOverlay` descriptors
- inert quarantine descriptors for known-but-unsupported classes
- validation results that state accepted, narrowed, quarantined, or rejected
- audit metadata that links each mapped descriptor back to its source catalog registration and CORTEX lifecycle decision

## Mapping rules

### `ImportedHelperSubagentDefinition`

Use this surface when an imported entry describes one helper capability that can be staged under exactly one `AraComponent`.

Required descriptor fields:

- `definition_id`
- `canonical_label`
- `provenance_aliases[]`
- `source_registration_ref`
- `runtime_class`
- `host_targets[]`
- `compatible_component_roles[]`
- `allowed_input_contracts[]`
- `declared_output_contracts[]`
- `promotion_policy`
- `compatibility_version`

Rules:

- helper definitions remain definition-only until a native CORTEX bind or spawn action creates a `SubagentInstance`
- helper definitions may not increase the per-component subagent ceiling beyond `12`
- helper labels may expose provenance aliases, but CORTEX canonical labels remain the default display label
- helper host-target support is descriptive and never self-authorizing

### `ImportedArrayTemplate`

Use this surface when an imported entry describes a reusable component layout or helper-placement pattern.

Required descriptor fields:

- `template_id`
- `canonical_label`
- `source_registration_ref`
- `array_shape`
- `component_role_defaults[]`
- `helper_definition_refs[]`
- `owner_binding_policy`
- `lifecycle_gate_policy`
- `compatibility_version`

Rules:

- two-component and three-component templates are first-class
- larger templates require an explicit native construction rule or operator justification
- templates may seed construction, but the resulting `Character` and `AraComponent` records are still CORTEX-owned
- templates may suggest component roles, but they may not override owner resolution or lifecycle gates

### `ImportedCharacterOverlay`

Use this surface when an imported entry describes presentation, bounded behavior, or governance metadata attached to an existing character.

Required descriptor fields:

- `overlay_id`
- `canonical_label`
- `source_registration_ref`
- `target_character_policy`
- `overlay_kind`
- `allowed_attachment_points[]`
- `blocked_authority_claims[]`
- `prism_compatibility_ref?`
- `compatibility_version`

Rules:

- overlays may modify presentation or bounded behavior only
- overlays may not replace character ownership, lifecycle state, canonical labels, or ARA component authority
- overlays that affect personality or stylization must include `prism_compatibility_ref` and remain compatible with the `PRISM -> CORTEX` boundary
- overlays with unresolved target policies remain staged or quarantined

## Lifecycle ownership

- imported descriptors start as `staged`
- descriptors move to `validated` only after provenance, compatibility, and authority checks pass
- descriptors move to `bound` only when attached to an eligible character or component by a native CORTEX action
- helper definitions create `active` runtime behavior only through a `SubagentInstance`
- descriptors move to `retired` without deleting provenance or audit metadata

CORTEX owns lifecycle state after import. Source repositories own the source catalog content, but they do not own CORTEX binding, promotion, activation, or retirement decisions.

## Validation rules

- every imported descriptor must resolve to a known `CortexCatalogRegistration`
- exactly one target surface must be selected for each accepted descriptor
- unresolved source provenance must reject the mapping
- unknown runtime classes must be quarantined or rejected, not silently narrowed
- helper definitions must name at least one compatible component role
- array templates must provide a valid array shape and non-conflicting component role defaults
- overlays must declare blocked authority claims and fail if they try to replace identity, owner, lifecycle, or label authority
- binding must fail if the target component would exceed its subagent ceiling

## Error behavior

Stable native error families for imported mapping:

- `CORTEX_ERR_IMPORT_PROVENANCE_INVALID`
- `CORTEX_ERR_IMPORT_CLASS_UNSUPPORTED`
- `CORTEX_ERR_IMPORT_SURFACE_AMBIGUOUS`
- `CORTEX_ERR_IMPORT_REFERENCE_UNRESOLVED`
- `CORTEX_ERR_IMPORT_ARRAY_POLICY_INVALID`
- `CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION`
- `CORTEX_ERR_IMPORT_COMPONENT_INCOMPATIBLE`
- `CORTEX_ERR_IMPORT_PROMOTION_REQUIRED`

Each failure should report:

- imported descriptor id
- source registration id
- selected or attempted target surface
- failing rule id
- whether the descriptor was rejected or quarantined
- operator-fixable remediation when narrowing can recover the mapping

Mapping failures must not partially mutate characters, components, subagent instances, overlays, or lifecycle state.

## Compatibility and versioning notes

- descriptor versions are additive by default
- existing field meanings must not be reassigned across versions
- unknown descriptor versions should be quarantined until a native mapper can classify them
- `compatibility_version` belongs to the mapped CORTEX descriptor; source package versions remain provenance data
- future runtime implementation should expose mapper and descriptor versions through the stable C ABI

## Example usage

```text
1. CORTEX receives a CortexCatalogRegistration for an external helper from SYMBIOSIS with source_repo, source_path, and source_commit populated.
2. The mapper classifies runtime_class=helper as ImportedHelperSubagentDefinition and validates host_targets against the external integration contract.
3. The descriptor stays staged until provenance and compatibility checks pass.
4. A two-component ImportedArrayTemplate references the helper for the active component role.
5. The operator creates a Character from the template; CORTEX creates native AraComponent records and keeps ownership in CORTEX.
6. The helper becomes runtime-active only after CORTEX binds it as a SubagentInstance under one eligible component.
7. If an overlay tries to replace the character owner, CORTEX rejects it with CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION.
```
