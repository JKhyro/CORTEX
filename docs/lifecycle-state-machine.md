# CORTEX Lifecycle State Machine

Issue alignment: `CORTEX #5`

## Purpose

Define the authoritative lifecycle state machine for CORTEX characters, ARA components, and subagent instances.

The state machine is owned by the native CORTEX core. UI shells, import lanes, and integration surfaces may request transitions, but they do not decide them.

## Interface contract

The lifecycle contract covers three targets:

- `Character`
- `AraComponent`
- `SubagentInstance`

Each target exposes:

- a current lifecycle state
- a bounded set of legal control verbs
- validation guards
- transition events and failure codes

## Expected inputs and outputs

Inputs:

- native control verbs from operator or automation surfaces
- validation results from the canonical entity model
- bounded personality or progression updates that CORTEX has already accepted
- runtime signals such as spawn success, health degradation, or explicit archive requests

Outputs:

- new lifecycle state
- audit events
- downstream notifications for UI and packaging layers
- structured transition failures

## Character states

- `draft`: character record exists but required ownership or component binding is incomplete
- `assembling`: components are being attached or adjusted
- `validated`: entity checks passed and the character is structurally sound
- `ready`: character is approved for activation but not yet live
- `active`: character is live and allowed to execute governed behavior
- `suspended`: character is intentionally paused without being destroyed
- `degraded`: character remains present but one or more components are impaired
- `archived`: character is inactive, immutable for runtime purposes, and kept for history or recovery

## Component states

- `staged`: slot exists but is not yet fully bound
- `bound`: owner, role, and definition references are attached
- `validated`: slot-level checks passed
- `ready`: component is cleared to activate with the parent character
- `active`: component is live
- `suspended`: component is intentionally paused
- `degraded`: component remains present but has limited capability
- `detached`: component was removed from active composition but retained for audit
- `archived`: component is permanently inactive

## Subagent states

- `defined`: instance record exists but the runtime process is not yet requested
- `queued`: spawn request is accepted and waiting for execution
- `active`: subagent process is present and governed
- `suspended`: instance is retained but paused
- `despawned`: runtime process is gone while the audit record remains
- `archived`: instance record is frozen and no longer eligible for reactivation

## Control verbs

### Character verbs

- `create_character`
- `bind_component`
- `validate_character`
- `mark_ready`
- `activate_character`
- `suspend_character`
- `resume_character`
- `degrade_character`
- `restore_character`
- `archive_character`

### Component verbs

- `stage_component`
- `bind_component_role`
- `validate_component`
- `activate_component`
- `suspend_component`
- `degrade_component`
- `restore_component`
- `detach_component`
- `archive_component`

### Subagent verbs

- `define_subagent`
- `queue_subagent_spawn`
- `confirm_subagent_spawn`
- `suspend_subagent`
- `resume_subagent`
- `despawn_subagent`
- `archive_subagent`

## Transition rules

- `draft -> assembling`: allowed after owner identity is resolved
- `assembling -> validated`: allowed only when all required components are bound and the entity model validates
- `validated -> ready`: allowed only after lifecycle guards, destructive-action protections, and import checks pass
- `ready -> active`: allowed only when every required component is `ready`
- `active -> suspended`: allowed by explicit operator action or governance safety policy
- `active -> degraded`: allowed when a required component or governed dependency is impaired
- `degraded -> active`: allowed only after restoring required component health
- `suspended -> active`: allowed only after revalidation if any component or import binding changed while suspended
- `active|suspended|degraded -> archived`: allowed only after all live subagent instances are `despawned` or `archived`

## Cross-target safeguards

- A component may not enter `active` unless its parent character is `ready` or `active`.
- A subagent may not enter `queued` or `active` unless its owning component is `ready`, `active`, or `degraded` and still below the per-component ceiling.
- No component may own more than `12` live or queued subagents.
- `PRISM` and `ASCENT` inputs may influence policy or display, but they may not force lifecycle transitions by themselves.
- Imported definitions may not bypass validation or archive safeguards.

## Error behavior

Stable lifecycle error families:

- `CORTEX_ERR_INVALID_TRANSITION`
- `CORTEX_ERR_TARGET_NOT_READY`
- `CORTEX_ERR_PARENT_STATE_CONFLICT`
- `CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED`
- `CORTEX_ERR_VALIDATION_REQUIRED`
- `CORTEX_ERR_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN`

Each failure should report:

- target entity id
- current state
- requested verb
- violated guard
- operator-fixable remediation when applicable

## Compatibility and versioning notes

- State names are part of the native contract and must remain stable once exposed.
- New states may be added only when they do not change existing transition meaning.
- Control verbs must stay explicit at the C ABI boundary.
- UI hosts should treat unknown future states as non-authoritative display values and defer action eligibility back to the native core.

## Example usage

```text
1. create_character -> draft
2. bind_component(active), bind_component(memory) -> assembling
3. validate_character -> validated
4. mark_ready -> ready
5. activate_character -> active
6. queue_subagent_spawn on component 0 -> queued
7. confirm_subagent_spawn -> active
8. degrade_component(memory) -> component degraded, character degraded
9. restore_component(memory) -> component active, character active
10. despawn all subagents, suspend_character, archive_character -> archived
```
