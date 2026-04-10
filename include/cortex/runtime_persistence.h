#ifndef CORTEX_RUNTIME_PERSISTENCE_H
#define CORTEX_RUNTIME_PERSISTENCE_H

#include "cortex/character_runtime.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CORTEX_RUNTIME_PERSISTENCE_ABI_VERSION_MAJOR 1u
#define CORTEX_RUNTIME_PERSISTENCE_ABI_VERSION_MINOR 0u
#define CORTEX_RUNTIME_PERSISTENCE_ABI_VERSION_PATCH 0u

#define CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION 1u

#define CORTEX_PERSISTED_ID_CAPACITY 128u
#define CORTEX_PERSISTED_PATH_CAPACITY 256u
#define CORTEX_PERSISTED_MESSAGE_CAPACITY 256u

typedef enum cortex_persistence_error {
  CORTEX_PERSISTENCE_OK = 0,
  CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT = 1,
  CORTEX_ERR_PERSISTENCE_IO = 2,
  CORTEX_ERR_PERSISTENCE_CAPACITY_EXCEEDED = 3,
  CORTEX_ERR_PERSISTENCE_SCHEMA_MISMATCH = 4,
  CORTEX_ERR_PERSISTENCE_PARSE = 5,
  CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED = 6,
  CORTEX_ERR_PERSISTENCE_RUNTIME_REJECTED = 7
} cortex_persistence_error;

typedef struct cortex_persistence_result {
  cortex_persistence_error error;
  const char *rule_id;
  const char *message;
  size_t failing_index;
  size_t processed_count;
} cortex_persistence_result;

typedef struct cortex_persisted_import_audit {
  uint32_t schema_version;
  uint32_t descriptor_version;
  char registration_id[CORTEX_PERSISTED_ID_CAPACITY];
  char source_repo[CORTEX_PERSISTED_ID_CAPACITY];
  char source_path[CORTEX_PERSISTED_PATH_CAPACITY];
  char source_commit[CORTEX_PERSISTED_ID_CAPACITY];
  char descriptor_id[CORTEX_PERSISTED_ID_CAPACITY];
  char rule_id[CORTEX_PERSISTED_ID_CAPACITY];
  char message[CORTEX_PERSISTED_MESSAGE_CAPACITY];
  cortex_import_surface surface;
  cortex_import_outcome outcome;
  cortex_import_lifecycle_state lifecycle_state;
  cortex_import_error import_error;
  uint8_t definition_only;
  uint8_t can_bind_as_subagent;
} cortex_persisted_import_audit;

typedef struct cortex_persisted_character_record {
  char character_id[CORTEX_PERSISTED_ID_CAPACITY];
  char owner_identity_ref[CORTEX_PERSISTED_ID_CAPACITY];
  cortex_ara_shape array_shape;
  cortex_character_state lifecycle_state;
  size_t first_component_index;
  size_t component_count;
} cortex_persisted_character_record;

typedef struct cortex_persisted_component_record {
  char component_id[CORTEX_PERSISTED_ID_CAPACITY];
  char owner_identity_ref[CORTEX_PERSISTED_ID_CAPACITY];
  uint32_t slot_index;
  cortex_component_role role;
  cortex_component_state lifecycle_state;
  uint32_t subagent_limit;
  uint32_t live_or_queued_subagents;
} cortex_persisted_component_record;

typedef struct cortex_persisted_subagent_record {
  char subagent_id[CORTEX_PERSISTED_ID_CAPACITY];
  char definition_ref[CORTEX_PERSISTED_ID_CAPACITY];
  size_t component_index;
  cortex_subagent_state lifecycle_state;
  uint8_t imported_definition;
} cortex_persisted_subagent_record;

typedef struct cortex_runtime_state_snapshot {
  uint32_t schema_version;
  cortex_persisted_import_audit *audits;
  size_t audit_count;
  size_t audit_capacity;
  cortex_persisted_character_record *characters;
  size_t character_count;
  size_t character_capacity;
  cortex_persisted_component_record *components;
  size_t component_count;
  size_t component_capacity;
  cortex_persisted_subagent_record *subagents;
  size_t subagent_count;
  size_t subagent_capacity;
} cortex_runtime_state_snapshot;

uint32_t cortex_runtime_persistence_abi_version(void);
uint32_t cortex_runtime_persistence_schema_version(void);

cortex_persistence_error cortex_catalog_ingest_batch(
  const cortex_catalog_registration *registrations,
  size_t registration_count,
  cortex_persisted_import_audit *audit_records,
  size_t audit_capacity,
  size_t *audit_count,
  cortex_persistence_result *result);

cortex_persistence_error cortex_runtime_capture_character(
  const cortex_character *character,
  const cortex_subagent_instance *subagents,
  size_t subagent_count,
  cortex_runtime_state_snapshot *snapshot,
  cortex_persistence_result *result);

cortex_persistence_error cortex_runtime_state_save(
  const char *path,
  const cortex_runtime_state_snapshot *snapshot,
  cortex_persistence_result *result);

cortex_persistence_error cortex_runtime_state_load(
  const char *path,
  cortex_runtime_state_snapshot *snapshot,
  cortex_persistence_result *result);

const char *cortex_persistence_error_name(cortex_persistence_error error);

#ifdef __cplusplus
}
#endif

#endif
