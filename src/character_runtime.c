#include "cortex/character_runtime.h"

#define CORTEX_CHARACTER_DESCRIPTOR_VERSION 1u

static int is_empty(const char *value) {
  return value == 0 || value[0] == '\0';
}

static uint32_t component_limit(const cortex_ara_component *component) {
  return component->subagent_limit == 0u ? CORTEX_DEFAULT_SUBAGENT_LIMIT : component->subagent_limit;
}

static void clear_result(cortex_runtime_result *result) {
  if (result == 0) {
    return;
  }

  result->error = CORTEX_ERR_RUNTIME_INVALID_ARGUMENT;
  result->rule_id = "invalid_argument";
  result->message = "invalid runtime argument";
  result->failing_index = 0u;
}

static cortex_runtime_error finish(
  cortex_runtime_result *result,
  cortex_runtime_error error,
  const char *rule_id,
  const char *message,
  size_t failing_index) {
  if (result != 0) {
    result->error = error;
    result->rule_id = rule_id;
    result->message = message;
    result->failing_index = failing_index;
  }

  return error;
}

static int is_valid_role(cortex_component_role role) {
  return role == CORTEX_COMPONENT_ROLE_ACTIVE ||
         role == CORTEX_COMPONENT_ROLE_MEMORY ||
         role == CORTEX_COMPONENT_ROLE_REASONING;
}

static int component_can_mark_ready(cortex_component_state state) {
  return state == CORTEX_COMPONENT_VALIDATED ||
         state == CORTEX_COMPONENT_READY ||
         state == CORTEX_COMPONENT_ACTIVE;
}

static int component_can_activate_with_character(cortex_component_state state) {
  return state == CORTEX_COMPONENT_READY || state == CORTEX_COMPONENT_ACTIVE;
}

static int component_allows_subagent_queue(cortex_component_state state) {
  return state == CORTEX_COMPONENT_READY ||
         state == CORTEX_COMPONENT_ACTIVE ||
         state == CORTEX_COMPONENT_DEGRADED;
}

static int subagent_is_live_child(cortex_subagent_state state) {
  return state == CORTEX_SUBAGENT_QUEUED ||
         state == CORTEX_SUBAGENT_ACTIVE ||
         state == CORTEX_SUBAGENT_SUSPENDED;
}

static int character_has_live_children(const cortex_character *character) {
  size_t index = 0u;

  for (index = 0u; index < character->component_count; ++index) {
    if (character->components[index].live_or_queued_subagents > 0u) {
      return 1;
    }
  }

  return 0;
}

static int expected_component_count(cortex_ara_shape shape, size_t actual_count) {
  if (shape == CORTEX_ARA_SHAPE_TWO_COMPONENT) {
    return actual_count == 2u;
  }

  if (shape == CORTEX_ARA_SHAPE_THREE_COMPONENT) {
    return actual_count == 3u;
  }

  if (shape == CORTEX_ARA_SHAPE_LARGER_JUSTIFIED) {
    return actual_count > 3u;
  }

  return 0;
}

static int has_role(const cortex_character *character, cortex_component_role role) {
  size_t index = 0u;

  for (index = 0u; index < character->component_count; ++index) {
    if (character->components[index].role == role) {
      return 1;
    }
  }

  return 0;
}

static int has_required_role_set(const cortex_character *character) {
  if (character->array_shape == CORTEX_ARA_SHAPE_TWO_COMPONENT) {
    return has_role(character, CORTEX_COMPONENT_ROLE_ACTIVE) &&
           has_role(character, CORTEX_COMPONENT_ROLE_MEMORY);
  }

  if (character->array_shape == CORTEX_ARA_SHAPE_THREE_COMPONENT) {
    return has_role(character, CORTEX_COMPONENT_ROLE_ACTIVE) &&
           has_role(character, CORTEX_COMPONENT_ROLE_MEMORY) &&
           has_role(character, CORTEX_COMPONENT_ROLE_REASONING);
  }

  return 1;
}

uint32_t cortex_character_runtime_abi_version(void) {
  return (CORTEX_CHARACTER_RUNTIME_ABI_VERSION_MAJOR << 16) |
         (CORTEX_CHARACTER_RUNTIME_ABI_VERSION_MINOR << 8) |
         CORTEX_CHARACTER_RUNTIME_ABI_VERSION_PATCH;
}

uint32_t cortex_character_runtime_descriptor_version(void) {
  return CORTEX_CHARACTER_DESCRIPTOR_VERSION;
}

cortex_runtime_error cortex_character_validate(
  const cortex_character *character,
  cortex_runtime_result *result) {
  size_t left = 0u;
  size_t right = 0u;

  clear_result(result);

  if (character == 0 || character->components == 0 || character->component_count == 0u) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_ARGUMENT,
                  "character_required",
                  "character and component descriptors are required", 0u);
  }

  if (is_empty(character->character_id) || is_empty(character->owner_identity_ref)) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_OWNER,
                  "character_owner_required",
                  "character id and owner identity are required", 0u);
  }

  if (!expected_component_count(character->array_shape, character->component_count)) {
    return finish(result, CORTEX_ERR_RUNTIME_UNSUPPORTED_ARRAY_SHAPE,
                  "array_shape_unsupported",
                  "array shape must match two-component, three-component, or justified larger count", 0u);
  }

  for (left = 0u; left < character->component_count; ++left) {
    const cortex_ara_component *component = &character->components[left];

    if (is_empty(component->component_id) || is_empty(component->owner_identity_ref)) {
      return finish(result, CORTEX_ERR_RUNTIME_INVALID_OWNER,
                    "component_owner_required",
                    "component id and owner identity are required", left);
    }

    if (!is_valid_role(component->role)) {
      return finish(result, CORTEX_ERR_RUNTIME_INVALID_COMPONENT_ROLE,
                    "component_role_invalid",
                    "component role must be active, memory, or reasoning", left);
    }

    if (component->live_or_queued_subagents > component_limit(component)) {
      return finish(result, CORTEX_ERR_RUNTIME_SUBAGENT_LIMIT_EXCEEDED,
                    "component_subagent_limit_exceeded",
                    "component has more live or queued subagents than its limit", left);
    }

    for (right = left + 1u; right < character->component_count; ++right) {
      if (component->slot_index == character->components[right].slot_index) {
        return finish(result, CORTEX_ERR_RUNTIME_DUPLICATE_SLOT,
                      "component_slot_duplicate",
                      "component slot indexes must be unique", right);
      }
    }
  }

  if (!has_required_role_set(character)) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_COMPONENT_ROLE,
                  "array_required_roles_missing",
                  "two-component ARAs require active and memory roles; three-component ARAs also require reasoning",
                  0u);
  }

  return finish(result, CORTEX_RUNTIME_OK,
                "character_validated",
                "character, component, and array descriptors are valid", 0u);
}

cortex_runtime_error cortex_character_apply_verb(
  cortex_character *character,
  cortex_control_verb verb,
  cortex_runtime_result *result) {
  cortex_runtime_error validation = CORTEX_RUNTIME_OK;
  size_t index = 0u;

  clear_result(result);

  if (character == 0) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_ARGUMENT,
                  "character_required",
                  "character descriptor is required", 0u);
  }

  if (verb == CORTEX_VERB_VALIDATE_CHARACTER) {
    validation = cortex_character_validate(character, result);
    if (validation != CORTEX_RUNTIME_OK) {
      return validation;
    }
    character->lifecycle_state = CORTEX_CHARACTER_VALIDATED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "character_validated",
                  "character moved to validated", 0u);
  }

  if (verb == CORTEX_VERB_MARK_READY) {
    if (character->lifecycle_state != CORTEX_CHARACTER_VALIDATED) {
      return finish(result, CORTEX_ERR_RUNTIME_INVALID_TRANSITION,
                    "character_ready_requires_validated",
                    "character must be validated before ready", 0u);
    }

    for (index = 0u; index < character->component_count; ++index) {
      if (!component_can_mark_ready(character->components[index].lifecycle_state)) {
        return finish(result, CORTEX_ERR_RUNTIME_TARGET_NOT_READY,
                      "component_not_ready",
                      "all components must be validated or ready before character ready", index);
      }
    }

    character->lifecycle_state = CORTEX_CHARACTER_READY;
    return finish(result, CORTEX_RUNTIME_OK,
                  "character_marked_ready",
                  "character moved to ready", 0u);
  }

  if (verb == CORTEX_VERB_ACTIVATE_CHARACTER) {
    if (character->lifecycle_state != CORTEX_CHARACTER_READY) {
      return finish(result, CORTEX_ERR_RUNTIME_INVALID_TRANSITION,
                    "character_activate_requires_ready",
                    "character must be ready before activation", 0u);
    }

    for (index = 0u; index < character->component_count; ++index) {
      if (!component_can_activate_with_character(character->components[index].lifecycle_state)) {
        return finish(result, CORTEX_ERR_RUNTIME_TARGET_NOT_READY,
                      "component_activation_not_ready",
                      "all required components must be ready before activation", index);
      }
    }

    character->lifecycle_state = CORTEX_CHARACTER_ACTIVE;
    for (index = 0u; index < character->component_count; ++index) {
      character->components[index].lifecycle_state = CORTEX_COMPONENT_ACTIVE;
    }
    return finish(result, CORTEX_RUNTIME_OK,
                  "character_activated",
                  "character and ready components moved to active", 0u);
  }

  if (verb == CORTEX_VERB_SUSPEND_CHARACTER &&
      (character->lifecycle_state == CORTEX_CHARACTER_ACTIVE ||
       character->lifecycle_state == CORTEX_CHARACTER_DEGRADED)) {
    character->lifecycle_state = CORTEX_CHARACTER_SUSPENDED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "character_suspended",
                  "character moved to suspended", 0u);
  }

  if (verb == CORTEX_VERB_RESUME_CHARACTER &&
      character->lifecycle_state == CORTEX_CHARACTER_SUSPENDED) {
    character->lifecycle_state = CORTEX_CHARACTER_ACTIVE;
    return finish(result, CORTEX_RUNTIME_OK,
                  "character_resumed",
                  "character moved to active", 0u);
  }

  if (verb == CORTEX_VERB_DEGRADE_CHARACTER &&
      character->lifecycle_state == CORTEX_CHARACTER_ACTIVE) {
    character->lifecycle_state = CORTEX_CHARACTER_DEGRADED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "character_degraded",
                  "character moved to degraded", 0u);
  }

  if (verb == CORTEX_VERB_RESTORE_CHARACTER &&
      character->lifecycle_state == CORTEX_CHARACTER_DEGRADED) {
    character->lifecycle_state = CORTEX_CHARACTER_ACTIVE;
    return finish(result, CORTEX_RUNTIME_OK,
                  "character_restored",
                  "character moved to active", 0u);
  }

  if (verb == CORTEX_VERB_ARCHIVE_CHARACTER) {
    if (character_has_live_children(character)) {
      return finish(result, CORTEX_ERR_RUNTIME_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN,
                    "archive_blocked_by_live_children",
                    "character cannot archive while components have live or queued subagents", 0u);
    }

    character->lifecycle_state = CORTEX_CHARACTER_ARCHIVED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "character_archived",
                  "character moved to archived", 0u);
  }

  return finish(result, CORTEX_ERR_RUNTIME_INVALID_TRANSITION,
                "character_transition_invalid",
                "requested character transition is not legal from the current state", 0u);
}

cortex_runtime_error cortex_component_apply_verb(
  cortex_character *character,
  size_t component_index,
  cortex_control_verb verb,
  cortex_runtime_result *result) {
  cortex_ara_component *component = 0;

  clear_result(result);

  if (character == 0 || character->components == 0 || component_index >= character->component_count) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_ARGUMENT,
                  "component_required",
                  "component index must resolve inside the character", component_index);
  }

  component = &character->components[component_index];

  if (verb == CORTEX_VERB_BIND_COMPONENT_ROLE && component->lifecycle_state == CORTEX_COMPONENT_STAGED) {
    if (!is_valid_role(component->role) || is_empty(component->owner_identity_ref)) {
      return finish(result, CORTEX_ERR_RUNTIME_INVALID_COMPONENT_ROLE,
                    "component_bind_invalid",
                    "component role and owner must be valid before binding", component_index);
    }
    component->lifecycle_state = CORTEX_COMPONENT_BOUND;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_bound",
                  "component role binding accepted", component_index);
  }

  if (verb == CORTEX_VERB_VALIDATE_COMPONENT && component->lifecycle_state == CORTEX_COMPONENT_BOUND) {
    if (component->live_or_queued_subagents > component_limit(component)) {
      return finish(result, CORTEX_ERR_RUNTIME_SUBAGENT_LIMIT_EXCEEDED,
                    "component_subagent_limit_exceeded",
                    "component exceeds subagent limit", component_index);
    }
    component->lifecycle_state = CORTEX_COMPONENT_VALIDATED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_validated",
                  "component moved to validated", component_index);
  }

  if (verb == CORTEX_VERB_MARK_COMPONENT_READY &&
      component->lifecycle_state == CORTEX_COMPONENT_VALIDATED) {
    component->lifecycle_state = CORTEX_COMPONENT_READY;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_ready",
                  "component moved to ready", component_index);
  }

  if (verb == CORTEX_VERB_ACTIVATE_COMPONENT && component->lifecycle_state == CORTEX_COMPONENT_READY) {
    if (character->lifecycle_state != CORTEX_CHARACTER_READY &&
        character->lifecycle_state != CORTEX_CHARACTER_ACTIVE) {
      return finish(result, CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT,
                    "component_parent_not_ready",
                    "parent character must be ready or active before component activation", component_index);
    }
    component->lifecycle_state = CORTEX_COMPONENT_ACTIVE;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_activated",
                  "component moved to active", component_index);
  }

  if (verb == CORTEX_VERB_SUSPEND_COMPONENT && component->lifecycle_state == CORTEX_COMPONENT_ACTIVE) {
    component->lifecycle_state = CORTEX_COMPONENT_SUSPENDED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_suspended",
                  "component moved to suspended", component_index);
  }

  if (verb == CORTEX_VERB_RESUME_COMPONENT && component->lifecycle_state == CORTEX_COMPONENT_SUSPENDED) {
    if (character->lifecycle_state != CORTEX_CHARACTER_READY &&
        character->lifecycle_state != CORTEX_CHARACTER_ACTIVE) {
      return finish(result, CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT,
                    "component_parent_not_resumable",
                    "parent character must be ready or active before component resume", component_index);
    }
    component->lifecycle_state = CORTEX_COMPONENT_ACTIVE;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_resumed",
                  "component moved to active", component_index);
  }

  if (verb == CORTEX_VERB_DEGRADE_COMPONENT && component->lifecycle_state == CORTEX_COMPONENT_ACTIVE) {
    component->lifecycle_state = CORTEX_COMPONENT_DEGRADED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_degraded",
                  "component moved to degraded", component_index);
  }

  if (verb == CORTEX_VERB_RESTORE_COMPONENT && component->lifecycle_state == CORTEX_COMPONENT_DEGRADED) {
    if (character->lifecycle_state != CORTEX_CHARACTER_READY &&
        character->lifecycle_state != CORTEX_CHARACTER_ACTIVE &&
        character->lifecycle_state != CORTEX_CHARACTER_DEGRADED) {
      return finish(result, CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT,
                    "component_parent_not_restorable",
                    "parent character must be ready, active, or degraded before component restore", component_index);
    }
    component->lifecycle_state = CORTEX_COMPONENT_ACTIVE;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_restored",
                  "component moved to active", component_index);
  }

  if (verb == CORTEX_VERB_DETACH_COMPONENT && component->lifecycle_state != CORTEX_COMPONENT_ARCHIVED) {
    if (component->live_or_queued_subagents != 0u) {
      return finish(result, CORTEX_ERR_RUNTIME_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN,
                    "component_detach_blocked_by_live_children",
                    "component cannot detach while subagents are live or queued", component_index);
    }
    component->lifecycle_state = CORTEX_COMPONENT_DETACHED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_detached",
                  "component moved to detached", component_index);
  }

  if (verb == CORTEX_VERB_ARCHIVE_COMPONENT && component->lifecycle_state != CORTEX_COMPONENT_ARCHIVED) {
    if (component->live_or_queued_subagents != 0u) {
      return finish(result, CORTEX_ERR_RUNTIME_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN,
                    "component_archive_blocked_by_live_children",
                    "component cannot archive while subagents are live or queued", component_index);
    }
    component->lifecycle_state = CORTEX_COMPONENT_ARCHIVED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "component_archived",
                  "component moved to archived", component_index);
  }

  return finish(result, CORTEX_ERR_RUNTIME_INVALID_TRANSITION,
                "component_transition_invalid",
                "requested component transition is not legal from the current state", component_index);
}

cortex_runtime_error cortex_subagent_bind_imported_helper(
  cortex_character *character,
  size_t component_index,
  const cortex_import_mapping_result *mapping,
  cortex_subagent_instance *subagent,
  cortex_runtime_result *result) {
  cortex_ara_component *component = 0;

  clear_result(result);

  if (character == 0 || character->components == 0 ||
      component_index >= character->component_count ||
      mapping == 0 || subagent == 0) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_ARGUMENT,
                  "subagent_bind_arguments_required",
                  "character, component, mapping, and subagent descriptors are required", component_index);
  }

  component = &character->components[component_index];

  if (subagent->lifecycle_state != CORTEX_SUBAGENT_DEFINED) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_TRANSITION,
                  "subagent_bind_requires_defined",
                  "only defined subagents can bind an imported helper", component_index);
  }

  if (mapping->error != CORTEX_IMPORT_OK ||
      mapping->surface != CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION ||
      mapping->definition_only == 0u ||
      mapping->can_bind_as_subagent == 0u) {
    return finish(result, CORTEX_ERR_RUNTIME_IMPORT_MAPPING_REJECTED,
                  "import_mapping_not_bindable",
                  "only accepted imported helper definitions may bind as subagent instances", component_index);
  }

  if (!component_allows_subagent_queue(component->lifecycle_state)) {
    return finish(result, CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT,
                  "component_state_blocks_subagent_queue",
                  "component must be ready, active, or degraded before queueing subagents", component_index);
  }

  if (component->live_or_queued_subagents >= component_limit(component)) {
    return finish(result, CORTEX_ERR_RUNTIME_SUBAGENT_LIMIT_EXCEEDED,
                  "component_subagent_limit_exceeded",
                  "component cannot queue another governed subagent", component_index);
  }

  subagent->definition_ref = mapping->descriptor_id;
  subagent->component_index = component_index;
  subagent->lifecycle_state = CORTEX_SUBAGENT_QUEUED;
  subagent->imported_definition = 1u;
  component->live_or_queued_subagents += 1u;

  return finish(result, CORTEX_RUNTIME_OK,
                "imported_helper_queued",
                "imported helper became runtime-relevant through a governed SubagentInstance", component_index);
}

static cortex_runtime_error apply_subagent_verb(
  cortex_subagent_instance *subagent,
  cortex_control_verb verb,
  cortex_runtime_result *result,
  int allow_owned_despawn) {
  clear_result(result);

  if (subagent == 0) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_ARGUMENT,
                  "subagent_required",
                  "subagent descriptor is required", 0u);
  }

  if (verb == CORTEX_VERB_CONFIRM_SUBAGENT_SPAWN &&
      subagent->lifecycle_state == CORTEX_SUBAGENT_QUEUED) {
    subagent->lifecycle_state = CORTEX_SUBAGENT_ACTIVE;
    return finish(result, CORTEX_RUNTIME_OK,
                  "subagent_spawn_confirmed",
                  "subagent moved to active", subagent->component_index);
  }

  if (verb == CORTEX_VERB_DESPAWN_SUBAGENT &&
      subagent_is_live_child(subagent->lifecycle_state)) {
    if (!allow_owned_despawn) {
      return finish(result, CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT,
                    "subagent_component_context_required",
                    "owned subagent despawn requires component context", subagent->component_index);
    }
    subagent->lifecycle_state = CORTEX_SUBAGENT_DESPAWNED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "subagent_despawned",
                  "subagent moved to despawned", subagent->component_index);
  }

  if (verb == CORTEX_VERB_SUSPEND_SUBAGENT &&
      subagent->lifecycle_state == CORTEX_SUBAGENT_ACTIVE) {
    subagent->lifecycle_state = CORTEX_SUBAGENT_SUSPENDED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "subagent_suspended",
                  "subagent moved to suspended", subagent->component_index);
  }

  if (verb == CORTEX_VERB_RESUME_SUBAGENT &&
      subagent->lifecycle_state == CORTEX_SUBAGENT_SUSPENDED) {
    subagent->lifecycle_state = CORTEX_SUBAGENT_ACTIVE;
    return finish(result, CORTEX_RUNTIME_OK,
                  "subagent_resumed",
                  "subagent moved to active", subagent->component_index);
  }

  if (verb == CORTEX_VERB_ARCHIVE_SUBAGENT &&
      (subagent->lifecycle_state == CORTEX_SUBAGENT_DESPAWNED ||
       subagent->lifecycle_state == CORTEX_SUBAGENT_DEFINED)) {
    subagent->lifecycle_state = CORTEX_SUBAGENT_ARCHIVED;
    return finish(result, CORTEX_RUNTIME_OK,
                  "subagent_archived",
                  "subagent moved to archived", subagent->component_index);
  }

  return finish(result, CORTEX_ERR_RUNTIME_INVALID_TRANSITION,
                "subagent_transition_invalid",
                "requested subagent transition is not legal from the current state", subagent->component_index);
}

cortex_runtime_error cortex_subagent_apply_verb(
  cortex_subagent_instance *subagent,
  cortex_control_verb verb,
  cortex_runtime_result *result) {
  return apply_subagent_verb(subagent, verb, result, 0);
}

cortex_runtime_error cortex_subagent_apply_verb_for_component(
  cortex_character *character,
  size_t component_index,
  cortex_subagent_instance *subagent,
  cortex_control_verb verb,
  cortex_runtime_result *result) {
  cortex_runtime_error outcome = CORTEX_RUNTIME_OK;
  cortex_ara_component *component = 0;
  cortex_subagent_state before = CORTEX_SUBAGENT_DEFINED;

  clear_result(result);

  if (character == 0 || character->components == 0 ||
      component_index >= character->component_count ||
      subagent == 0 ||
      subagent->component_index != component_index) {
    return finish(result, CORTEX_ERR_RUNTIME_INVALID_ARGUMENT,
                  "subagent_component_context_required",
                  "subagent transition requires a matching owning component", component_index);
  }

  component = &character->components[component_index];
  before = subagent->lifecycle_state;
  outcome = apply_subagent_verb(subagent, verb, result, 1);

  if (outcome == CORTEX_RUNTIME_OK &&
      verb == CORTEX_VERB_DESPAWN_SUBAGENT &&
      subagent_is_live_child(before) &&
      component->live_or_queued_subagents > 0u) {
    component->live_or_queued_subagents -= 1u;
  }

  return outcome;
}

const char *cortex_character_state_name(cortex_character_state state) {
  switch (state) {
    case CORTEX_CHARACTER_DRAFT: return "draft";
    case CORTEX_CHARACTER_ASSEMBLING: return "assembling";
    case CORTEX_CHARACTER_VALIDATED: return "validated";
    case CORTEX_CHARACTER_READY: return "ready";
    case CORTEX_CHARACTER_ACTIVE: return "active";
    case CORTEX_CHARACTER_SUSPENDED: return "suspended";
    case CORTEX_CHARACTER_DEGRADED: return "degraded";
    case CORTEX_CHARACTER_ARCHIVED: return "archived";
    default: return "unknown";
  }
}

const char *cortex_component_state_name(cortex_component_state state) {
  switch (state) {
    case CORTEX_COMPONENT_STAGED: return "staged";
    case CORTEX_COMPONENT_BOUND: return "bound";
    case CORTEX_COMPONENT_VALIDATED: return "validated";
    case CORTEX_COMPONENT_READY: return "ready";
    case CORTEX_COMPONENT_ACTIVE: return "active";
    case CORTEX_COMPONENT_SUSPENDED: return "suspended";
    case CORTEX_COMPONENT_DEGRADED: return "degraded";
    case CORTEX_COMPONENT_DETACHED: return "detached";
    case CORTEX_COMPONENT_ARCHIVED: return "archived";
    default: return "unknown";
  }
}

const char *cortex_subagent_state_name(cortex_subagent_state state) {
  switch (state) {
    case CORTEX_SUBAGENT_DEFINED: return "defined";
    case CORTEX_SUBAGENT_QUEUED: return "queued";
    case CORTEX_SUBAGENT_ACTIVE: return "active";
    case CORTEX_SUBAGENT_SUSPENDED: return "suspended";
    case CORTEX_SUBAGENT_DESPAWNED: return "despawned";
    case CORTEX_SUBAGENT_ARCHIVED: return "archived";
    default: return "unknown";
  }
}

const char *cortex_runtime_error_name(cortex_runtime_error error) {
  switch (error) {
    case CORTEX_RUNTIME_OK: return "CORTEX_RUNTIME_OK";
    case CORTEX_ERR_RUNTIME_INVALID_ARGUMENT: return "CORTEX_ERR_RUNTIME_INVALID_ARGUMENT";
    case CORTEX_ERR_RUNTIME_INVALID_OWNER: return "CORTEX_ERR_RUNTIME_INVALID_OWNER";
    case CORTEX_ERR_RUNTIME_INVALID_COMPONENT_ROLE: return "CORTEX_ERR_RUNTIME_INVALID_COMPONENT_ROLE";
    case CORTEX_ERR_RUNTIME_DUPLICATE_SLOT: return "CORTEX_ERR_RUNTIME_DUPLICATE_SLOT";
    case CORTEX_ERR_RUNTIME_UNSUPPORTED_ARRAY_SHAPE: return "CORTEX_ERR_RUNTIME_UNSUPPORTED_ARRAY_SHAPE";
    case CORTEX_ERR_RUNTIME_INVALID_TRANSITION: return "CORTEX_ERR_RUNTIME_INVALID_TRANSITION";
    case CORTEX_ERR_RUNTIME_TARGET_NOT_READY: return "CORTEX_ERR_RUNTIME_TARGET_NOT_READY";
    case CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT: return "CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT";
    case CORTEX_ERR_RUNTIME_SUBAGENT_LIMIT_EXCEEDED: return "CORTEX_ERR_RUNTIME_SUBAGENT_LIMIT_EXCEEDED";
    case CORTEX_ERR_RUNTIME_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN:
      return "CORTEX_ERR_RUNTIME_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN";
    case CORTEX_ERR_RUNTIME_IMPORT_MAPPING_REJECTED: return "CORTEX_ERR_RUNTIME_IMPORT_MAPPING_REJECTED";
    default: return "CORTEX_ERR_RUNTIME_UNKNOWN";
  }
}
