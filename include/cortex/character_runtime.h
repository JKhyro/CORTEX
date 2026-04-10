#ifndef CORTEX_CHARACTER_RUNTIME_H
#define CORTEX_CHARACTER_RUNTIME_H

#include "cortex/imported_catalog_mapper.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CORTEX_CHARACTER_RUNTIME_ABI_VERSION_MAJOR 1u
#define CORTEX_CHARACTER_RUNTIME_ABI_VERSION_MINOR 0u
#define CORTEX_CHARACTER_RUNTIME_ABI_VERSION_PATCH 0u

typedef enum cortex_ara_shape {
  CORTEX_ARA_SHAPE_TWO_COMPONENT = 2,
  CORTEX_ARA_SHAPE_THREE_COMPONENT = 3,
  CORTEX_ARA_SHAPE_LARGER_JUSTIFIED = 4
} cortex_ara_shape;

typedef enum cortex_component_role {
  CORTEX_COMPONENT_ROLE_INVALID = 0,
  CORTEX_COMPONENT_ROLE_ACTIVE = 1,
  CORTEX_COMPONENT_ROLE_MEMORY = 2,
  CORTEX_COMPONENT_ROLE_REASONING = 3
} cortex_component_role;

typedef enum cortex_character_state {
  CORTEX_CHARACTER_DRAFT = 0,
  CORTEX_CHARACTER_ASSEMBLING = 1,
  CORTEX_CHARACTER_VALIDATED = 2,
  CORTEX_CHARACTER_READY = 3,
  CORTEX_CHARACTER_ACTIVE = 4,
  CORTEX_CHARACTER_SUSPENDED = 5,
  CORTEX_CHARACTER_DEGRADED = 6,
  CORTEX_CHARACTER_ARCHIVED = 7
} cortex_character_state;

typedef enum cortex_component_state {
  CORTEX_COMPONENT_STAGED = 0,
  CORTEX_COMPONENT_BOUND = 1,
  CORTEX_COMPONENT_VALIDATED = 2,
  CORTEX_COMPONENT_READY = 3,
  CORTEX_COMPONENT_ACTIVE = 4,
  CORTEX_COMPONENT_SUSPENDED = 5,
  CORTEX_COMPONENT_DEGRADED = 6,
  CORTEX_COMPONENT_DETACHED = 7,
  CORTEX_COMPONENT_ARCHIVED = 8
} cortex_component_state;

typedef enum cortex_subagent_state {
  CORTEX_SUBAGENT_DEFINED = 0,
  CORTEX_SUBAGENT_QUEUED = 1,
  CORTEX_SUBAGENT_ACTIVE = 2,
  CORTEX_SUBAGENT_SUSPENDED = 3,
  CORTEX_SUBAGENT_DESPAWNED = 4,
  CORTEX_SUBAGENT_ARCHIVED = 5
} cortex_subagent_state;

typedef enum cortex_control_verb {
  CORTEX_VERB_VALIDATE_CHARACTER = 0,
  CORTEX_VERB_MARK_READY = 1,
  CORTEX_VERB_ACTIVATE_CHARACTER = 2,
  CORTEX_VERB_SUSPEND_CHARACTER = 3,
  CORTEX_VERB_RESUME_CHARACTER = 4,
  CORTEX_VERB_DEGRADE_CHARACTER = 5,
  CORTEX_VERB_RESTORE_CHARACTER = 6,
  CORTEX_VERB_ARCHIVE_CHARACTER = 7,
  CORTEX_VERB_BIND_COMPONENT_ROLE = 8,
  CORTEX_VERB_VALIDATE_COMPONENT = 9,
  CORTEX_VERB_MARK_COMPONENT_READY = 10,
  CORTEX_VERB_ACTIVATE_COMPONENT = 11,
  CORTEX_VERB_CONFIRM_SUBAGENT_SPAWN = 12,
  CORTEX_VERB_DESPAWN_SUBAGENT = 13,
  CORTEX_VERB_ARCHIVE_SUBAGENT = 14,
  CORTEX_VERB_SUSPEND_COMPONENT = 15,
  CORTEX_VERB_RESUME_COMPONENT = 16,
  CORTEX_VERB_DEGRADE_COMPONENT = 17,
  CORTEX_VERB_RESTORE_COMPONENT = 18,
  CORTEX_VERB_DETACH_COMPONENT = 19,
  CORTEX_VERB_ARCHIVE_COMPONENT = 20,
  CORTEX_VERB_SUSPEND_SUBAGENT = 21,
  CORTEX_VERB_RESUME_SUBAGENT = 22
} cortex_control_verb;

typedef enum cortex_runtime_error {
  CORTEX_RUNTIME_OK = 0,
  CORTEX_ERR_RUNTIME_INVALID_ARGUMENT = 1,
  CORTEX_ERR_RUNTIME_INVALID_OWNER = 2,
  CORTEX_ERR_RUNTIME_INVALID_COMPONENT_ROLE = 3,
  CORTEX_ERR_RUNTIME_DUPLICATE_SLOT = 4,
  CORTEX_ERR_RUNTIME_UNSUPPORTED_ARRAY_SHAPE = 5,
  CORTEX_ERR_RUNTIME_INVALID_TRANSITION = 6,
  CORTEX_ERR_RUNTIME_TARGET_NOT_READY = 7,
  CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT = 8,
  CORTEX_ERR_RUNTIME_SUBAGENT_LIMIT_EXCEEDED = 9,
  CORTEX_ERR_RUNTIME_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN = 10,
  CORTEX_ERR_RUNTIME_IMPORT_MAPPING_REJECTED = 11
} cortex_runtime_error;

typedef struct cortex_ara_component {
  const char *component_id;
  uint32_t slot_index;
  cortex_component_role role;
  const char *owner_identity_ref;
  cortex_component_state lifecycle_state;
  uint32_t subagent_limit;
  uint32_t live_or_queued_subagents;
} cortex_ara_component;

typedef struct cortex_character {
  const char *character_id;
  const char *owner_identity_ref;
  cortex_ara_shape array_shape;
  cortex_character_state lifecycle_state;
  cortex_ara_component *components;
  size_t component_count;
} cortex_character;

typedef struct cortex_subagent_instance {
  const char *subagent_id;
  const char *definition_ref;
  size_t component_index;
  cortex_subagent_state lifecycle_state;
  uint8_t imported_definition;
} cortex_subagent_instance;

typedef struct cortex_runtime_result {
  cortex_runtime_error error;
  const char *rule_id;
  const char *message;
  size_t failing_index;
} cortex_runtime_result;

uint32_t cortex_character_runtime_abi_version(void);
uint32_t cortex_character_runtime_descriptor_version(void);

cortex_runtime_error cortex_character_validate(
  const cortex_character *character,
  cortex_runtime_result *result);

cortex_runtime_error cortex_character_apply_verb(
  cortex_character *character,
  cortex_control_verb verb,
  cortex_runtime_result *result);

cortex_runtime_error cortex_component_apply_verb(
  cortex_character *character,
  size_t component_index,
  cortex_control_verb verb,
  cortex_runtime_result *result);

cortex_runtime_error cortex_subagent_bind_imported_helper(
  cortex_character *character,
  size_t component_index,
  const cortex_import_mapping_result *mapping,
  cortex_subagent_instance *subagent,
  cortex_runtime_result *result);

cortex_runtime_error cortex_subagent_apply_verb(
  cortex_subagent_instance *subagent,
  cortex_control_verb verb,
  cortex_runtime_result *result);

cortex_runtime_error cortex_subagent_apply_verb_for_component(
  cortex_character *character,
  size_t component_index,
  cortex_subagent_instance *subagent,
  cortex_control_verb verb,
  cortex_runtime_result *result);

const char *cortex_character_state_name(cortex_character_state state);
const char *cortex_component_state_name(cortex_component_state state);
const char *cortex_subagent_state_name(cortex_subagent_state state);
const char *cortex_runtime_error_name(cortex_runtime_error error);

#ifdef __cplusplus
}
#endif

#endif
