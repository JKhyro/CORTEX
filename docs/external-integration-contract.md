# CORTEX External Integration Contract

Issue alignment: `CORTEX #3`

## Purpose

Define how `CORTEX` exposes governed characters, components, and imported external-agent catalog references to `VECTOR`, `NEXUS`, and external execution surfaces without giving those surfaces ownership of CORTEX lifecycle or identity rules.

This contract establishes the integration boundary. The deeper mapping of imported external subagent definitions into helper, template, overlay, and lifecycle classes remains follow-on work for `#9`.

This contract assumes the entity model in [`canonical-entity-model.md`](./canonical-entity-model.md), the runtime topology in [`runtime-topology.md`](./runtime-topology.md), and the packaging identity rules in [`packaging-distribution-model.md`](./packaging-distribution-model.md).

## Interface contract

The external integration boundary must expose four governed export families:

- `CortexCharacterExport`: canonical character identity, lifecycle state, and governed component summaries exposed to downstream hosts
- `CortexComponentExport`: component-level role, owner, capacity, overlay, and lifecycle references
- `CortexCatalogRegistration`: imported external catalog entries or runtime-capability registrations that CORTEX can reference without promoting them to persistent identities
- `CortexExecutionTarget`: host-target compatibility descriptors for downstream surfaces such as `VECTOR`, `NEXUS`, `codex`, `openclaw`, `symbiosis-native`, and `acp-bridge`

Downstream consumers may read and request governed actions against these exports, but they do not become the authority for lifecycle, ownership, or array composition.

## Expected inputs and outputs

Inputs into the integration boundary:

- canonical CORTEX character and component descriptors
- imported external catalog metadata from the validated upstream source
- host-target support declarations and runtime-class metadata
- operator or system requests to register, expose, hide, or narrow external execution targets

Outputs from the integration boundary:

- normalized character and component export descriptors
- explicit registration records for imported catalog entries
- host-target compatibility summaries for each exported surface
- governed validation results showing whether an external export or registration was accepted, narrowed, rejected, or quarantined

## Integration surfaces

### `VECTOR`

- `VECTOR` may consume governed character and component exports as assignable workspace helpers.
- `VECTOR` may not redefine lifecycle, owner, or canonical identity fields.

### `NEXUS`

- `NEXUS` may consume host capability, registry, and launch metadata that CORTEX exposes.
- `NEXUS` may not promote imported external definitions into persistent CORTEX identities by default.

### External execution targets

- External targets such as `codex`, `openclaw`, `symbiosis-native`, and `acp-bridge` must be expressed through explicit host-target compatibility fields.
- External target support must remain descriptive and governed, not self-authorizing.

## Required contract fields

- `source_repo`
- `source_path`
- `source_commit`
- `runtime_class`
- `host_targets[]`
- `activation_state`
- `lifecycle_owner`
- `overlay_ref`
- `array_binding_policy`
- `provenance_ref`

## Validation rules

- An external registration is invalid if provenance does not resolve to a known source repo, path, and commit.
- An export is invalid if it attempts to replace canonical CORTEX identity, lifecycle, or ownership fields.
- A host-target declaration is invalid if it names unsupported targets or omits required runtime-class information.
- Imported external catalog entries must not become persistent Symbiotes or Curators by default.
- Overlay references must remain governed by CORTEX and any related PRISM boundary rules.
- Unknown runtime classes or unsupported host targets must be rejected or quarantined; they must not be silently accepted.

## Error behavior

Stable native error families for this integration boundary:

- `CORTEX_ERR_EXPORT_PROVENANCE_INVALID`
- `CORTEX_ERR_EXPORT_RUNTIME_CLASS_UNKNOWN`
- `CORTEX_ERR_EXPORT_HOST_TARGET_UNSUPPORTED`
- `CORTEX_ERR_EXPORT_AUTHORITY_VIOLATION`
- `CORTEX_ERR_EXPORT_OVERLAY_CONFLICT`
- `CORTEX_ERR_EXPORT_ARRAY_POLICY_CONFLICT`

Each failure should report:

- the failing export or registration id
- the failing host target or runtime class
- the violated rule id
- provenance metadata
- whether the failure is retryable after narrowing or requires source correction

## Compatibility and versioning notes

- Export descriptor versions must remain explicit and additive by default.
- Existing field meanings for runtime class, host target support, and lifecycle ownership must not be reassigned across versions.
- Packaging identity from `#8` is a dependency: downstream consumers should trust only exports that declare compatible artifact and ABI identity.
- The follow-on `#9` mapping lane may add stricter classification rules without weakening this top-level authority boundary.

## Example usage

```text
1. CORTEX imports a validated external catalog entry from a known repo, path, and commit.
2. CORTEX registers that entry as an external helper definition with host-target support for `codex` and `symbiosis-native`.
3. VECTOR requests governed helper visibility for a specific character.
4. CORTEX returns a normalized character export plus the narrowed external registration metadata.
5. VECTOR can assign the helper in a workspace, but lifecycle ownership and canonical identity remain with CORTEX.
6. If the catalog entry claims an unknown runtime class, CORTEX rejects it with an explicit export compatibility error.
```
