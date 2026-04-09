# CORTEX And ASCENT Integration Contract

Issue alignment: `CORTEX #11`

## Purpose

Define the bounded `ASCENT -> CORTEX` contract for progression state, achievement points, rewards, penalties, and character-facing governance signals.

`ASCENT` may describe how a governed character progresses or what gated rewards and penalties apply, but it does not gain character-construction authority, lifecycle authority, or subagent-governance authority.

This contract assumes the entity model in [`canonical-entity-model.md`](./canonical-entity-model.md), the lifecycle rules in [`lifecycle-state-machine.md`](./lifecycle-state-machine.md), and the ARA doctrine in [`character-ara-doctrine.md`](./character-ara-doctrine.md).

## Interface contract

The native CORTEX core must accept only five ASCENT-origin payload families:

- `AscentAchievementLedger`: achievement-point totals, tracked milestones, and earned progression signals for a character
- `AscentProgressionState`: current level, rank, tier, or advancement state that may influence governed character behavior
- `AscentRewardGrant`: bounded reward unlocks, bonuses, or permissions that CORTEX may accept, narrow, stage, or reject
- `AscentPenaltyRecord`: penalties, cooldowns, or degraded progression states that CORTEX must treat as governed inputs rather than as direct lifecycle verbs
- `AscentStateSnapshot`: consolidated progression and reward-state snapshots that the shell may render after CORTEX validation

ASCENT-origin payloads must be accepted only through a CORTEX-owned binding action. They are never self-authorizing.

## Expected inputs and outputs

Inputs from ASCENT into CORTEX:

- a target `character_id`
- optional `component_id` targets when a payload is component-scoped
- payload provenance including source profile, profile version, and compatibility version
- progression, reward, penalty, or achievement fields already normalized by the ASCENT boundary layer
- operator intent flags when the shell is attaching, replacing, or clearing ASCENT-origin state

Outputs from CORTEX:

- accepted `progression_binding_ref` records on the target character
- accepted `reward_state_ref` or `penalty_state_ref` records on the target character or component
- normalized read-only summaries for shell presentation
- validation results that say whether the payload was accepted, narrowed, rejected, or quarantined
- stable native error families when authority, range, or compatibility rules are violated

## Binding boundaries

ASCENT may provide:

- achievement-point totals and milestone progress for a character or component
- progression state such as rank, level band, or advancement tier
- reward grants, unlock hints, or bounded capability bonuses that still remain subject to CORTEX policy
- penalty or cooldown state that narrows legal actions without becoming a lifecycle verb
- consolidated state snapshots that help the shell display governed progression context

ASCENT may not provide:

- character creation commands
- owner or curator reassignment
- ARA slot allocation or reorder authority
- direct lifecycle transitions such as `activate`, `suspend`, `archive`, or `despawn`
- subagent spawn authority
- persistent identity promotion
- export-surface authorization for downstream repos or external agents

## Native binding model

### Character-level bindings

- One `AscentProgressionState` may be attached as the default progression view for a character.
- Multiple reward and penalty records may be staged, but CORTEX must expose the resolved precedence and active windows explicitly.
- One consolidated state snapshot may be rendered for shell presentation, but it must reflect only CORTEX-accepted ASCENT inputs.

### Component-level bindings

- Component-scoped ASCENT bindings are allowed only when the target `component_id` belongs to the target `character_id`.
- A component binding may bias reward eligibility, progression emphasis, or cooldown visibility for that component.
- Component bindings may not change the component owner, lifecycle state, or subagent ceiling.

### Reward and penalty handling

- Reward grants remain advisory until accepted by CORTEX.
- CORTEX may narrow or discard a reward grant when it conflicts with lifecycle policy, ownership doctrine, or export safety.
- Penalty records may narrow legal actions or mark a capability as gated, but they must not impersonate direct lifecycle verbs.

## Required fields by payload family

### `AscentAchievementLedger`

- `ledger_id`
- `compatibility_version`
- `target_character_id`
- `target_component_ids[]`
- `achievement_points`
- `milestones[]`
- `provenance_ref`

### `AscentProgressionState`

- `progression_state_id`
- `compatibility_version`
- `target_character_id`
- `target_component_ids[]`
- `level_band`
- `rank_label`
- `progression_axes[]`
- `provenance_ref`

### `AscentRewardGrant`

- `reward_grant_id`
- `compatibility_version`
- `target_character_id`
- `target_component_ids[]`
- `reward_class`
- `reward_effects[]`
- `eligibility_window`
- `provenance_ref`

### `AscentPenaltyRecord`

- `penalty_record_id`
- `compatibility_version`
- `target_character_id`
- `target_component_ids[]`
- `penalty_class`
- `penalty_effects[]`
- `cooldown_window`
- `provenance_ref`

### `AscentStateSnapshot`

- `state_snapshot_id`
- `compatibility_version`
- `target_character_id`
- `target_component_ids[]`
- `achievement_summary`
- `reward_state_summary`
- `penalty_state_summary`
- `provenance_ref`

## Validation rules

- An ASCENT-origin payload is invalid if the target character does not exist.
- A component-scoped payload is invalid if any referenced `component_id` is not part of the target character.
- An achievement or progression payload is invalid if it attempts to redefine owner, curator, lifecycle, or export fields.
- A reward grant is invalid if it attempts to bypass native lifecycle or safety policy.
- A penalty record is invalid if it tries to apply direct lifecycle verbs instead of governed gating state.
- A state snapshot is invalid if it contains unresolved raw ASCENT authority that CORTEX has not accepted.
- Unknown compatibility versions must be rejected or quarantined; they must not be silently applied.
- ASCENT-origin payloads may be attached in `draft`, `validated`, `ready`, and `active` states, but attachment alone may not force a lifecycle transition.

## Error behavior

Stable native error families for this contract:

- `CORTEX_ERR_ASCENT_VERSION_MISMATCH`
- `CORTEX_ERR_ASCENT_UNKNOWN_TARGET_CHARACTER`
- `CORTEX_ERR_ASCENT_UNKNOWN_COMPONENT_TARGET`
- `CORTEX_ERR_ASCENT_PROGRESS_RANGE_INVALID`
- `CORTEX_ERR_ASCENT_REWARD_POLICY_CONFLICT`
- `CORTEX_ERR_ASCENT_PENALTY_POLICY_CONFLICT`
- `CORTEX_ERR_ASCENT_AUTHORITY_VIOLATION`

Each failure should report:

- the failing payload family and payload id
- the failing target character or component id
- the violated rule id
- provenance metadata
- whether the payload may be retried after narrowing

## Compatibility and versioning notes

- ASCENT payload families are additive by default.
- Existing accepted field meanings must not be reassigned across versions.
- The stable C ABI should expose explicit descriptor versions for ASCENT-origin bindings and validation results.
- Unknown optional fields may be preserved in inert provenance storage, but they must not affect resolved native behavior until the core supports them.
- Downstream shells should render only the normalized CORTEX view of accepted ASCENT bindings, not the raw ASCENT payload as an authority surface.

## Example usage

```text
1. The operator selects an existing Character in CORTEX.
2. ASCENT provides an achievement ledger, one progression state, and one reward grant for that character.
3. CORTEX validates that all targeted component ids belong to the selected character.
4. CORTEX accepts the progression state, narrows one reward effect, and keeps a penalty record staged as governed cooldown state.
5. The shell shows the resolved ASCENT bindings as read-only accepted inputs.
6. The character stays in its current lifecycle state until a separate native lifecycle action is taken.
```
