#include "cortex/runtime_persistence.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CORTEX_STATE_MAGIC "CORTEX_RUNTIME_STATE"
#define CORTEX_STATE_LINE_CAPACITY 2048u
#define CORTEX_STATE_MAX_FIELDS 16u

static int is_empty(const char *value) {
  return value == 0 || value[0] == '\0';
}

static int is_portable_field(const char *value) {
  const unsigned char *cursor = (const unsigned char *)value;

  if (value == 0) {
    return 1;
  }

  while (*cursor != '\0') {
    if (*cursor == '\t' || *cursor == '\r' || *cursor == '\n' || *cursor == '\\') {
      return 0;
    }
    ++cursor;
  }

  return 1;
}

static int fits_string(const char *value, size_t capacity) {
  if (value == 0) {
    return 1;
  }

  return strlen(value) < capacity && is_portable_field(value);
}

static void copy_string(char *target, size_t capacity, const char *value) {
  size_t length = 0u;

  if (capacity == 0u) {
    return;
  }

  if (value == 0) {
    target[0] = '\0';
    return;
  }

  length = strlen(value);
  if (length >= capacity) {
    length = capacity - 1u;
  }

  (void)memcpy(target, value, length);
  target[length] = '\0';
}

static FILE *open_file(const char *path, const char *mode) {
  FILE *file = 0;

#if defined(_MSC_VER)
  if (fopen_s(&file, path, mode) != 0) {
    return 0;
  }
  return file;
#else
  return fopen(path, mode);
#endif
}

static void clear_result(cortex_persistence_result *result) {
  if (result == 0) {
    return;
  }

  result->error = CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT;
  result->rule_id = "invalid_argument";
  result->message = "invalid persistence argument";
  result->failing_index = 0u;
  result->processed_count = 0u;
}

static cortex_persistence_error finish(
  cortex_persistence_result *result,
  cortex_persistence_error error,
  const char *rule_id,
  const char *message,
  size_t failing_index,
  size_t processed_count) {
  if (result != 0) {
    result->error = error;
    result->rule_id = rule_id;
    result->message = message;
    result->failing_index = failing_index;
    result->processed_count = processed_count;
  }

  return error;
}

static cortex_persistence_error check_audit_fields(
  const cortex_catalog_registration *registration,
  const cortex_import_mapping_result *mapping,
  size_t index,
  cortex_persistence_result *result) {
  if (!fits_string(registration->registration_id, CORTEX_PERSISTED_ID_CAPACITY) ||
      !fits_string(registration->source_repo, CORTEX_PERSISTED_ID_CAPACITY) ||
      !fits_string(registration->source_path, CORTEX_PERSISTED_PATH_CAPACITY) ||
      !fits_string(registration->source_commit, CORTEX_PERSISTED_ID_CAPACITY) ||
      !fits_string(mapping->descriptor_id, CORTEX_PERSISTED_ID_CAPACITY) ||
      !fits_string(mapping->rule_id, CORTEX_PERSISTED_ID_CAPACITY) ||
      !fits_string(mapping->message, CORTEX_PERSISTED_MESSAGE_CAPACITY)) {
    return finish(result, CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED,
                  "audit_field_unsupported",
                  "audit string fields must fit fixed storage and avoid tabs, newlines, and backslashes",
                  index, index);
  }

  return CORTEX_PERSISTENCE_OK;
}

static cortex_persistence_error store_audit_record(
  const cortex_catalog_registration *registration,
  const cortex_import_mapping_result *mapping,
  cortex_persisted_import_audit *audit,
  size_t index,
  cortex_persistence_result *result) {
  cortex_persistence_error error = check_audit_fields(registration, mapping, index, result);
  if (error != CORTEX_PERSISTENCE_OK) {
    return error;
  }

  audit->schema_version = CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION;
  audit->descriptor_version = mapping->descriptor_version;
  copy_string(audit->registration_id, sizeof(audit->registration_id), registration->registration_id);
  copy_string(audit->source_repo, sizeof(audit->source_repo), registration->source_repo);
  copy_string(audit->source_path, sizeof(audit->source_path), registration->source_path);
  copy_string(audit->source_commit, sizeof(audit->source_commit), registration->source_commit);
  copy_string(audit->descriptor_id, sizeof(audit->descriptor_id), mapping->descriptor_id);
  copy_string(audit->rule_id, sizeof(audit->rule_id), mapping->rule_id);
  copy_string(audit->message, sizeof(audit->message), mapping->message);
  audit->surface = mapping->surface;
  audit->outcome = mapping->outcome;
  audit->lifecycle_state = mapping->lifecycle_state;
  audit->import_error = mapping->error;
  audit->definition_only = mapping->definition_only;
  audit->can_bind_as_subagent = mapping->can_bind_as_subagent;
  return CORTEX_PERSISTENCE_OK;
}

static size_t split_fields(char *line, char **fields, size_t field_capacity) {
  size_t count = 0u;
  char *cursor = line;

  while (count < field_capacity) {
    fields[count++] = cursor;
    cursor = strchr(cursor, '\t');
    if (cursor == 0) {
      break;
    }
    *cursor = '\0';
    ++cursor;
  }

  return count;
}

static void trim_line(char *line) {
  size_t length = strlen(line);
  while (length > 0u && (line[length - 1u] == '\n' || line[length - 1u] == '\r')) {
    line[length - 1u] = '\0';
    --length;
  }
}

static int parse_u32(const char *value, uint32_t *out) {
  unsigned long parsed = 0ul;
  char *end = 0;

  if (is_empty(value) || out == 0) {
    return 0;
  }

  parsed = strtoul(value, &end, 10);
  if (end == value || *end != '\0' || parsed > 0xfffffffful) {
    return 0;
  }

  *out = (uint32_t)parsed;
  return 1;
}

static int parse_size(const char *value, size_t *out) {
  unsigned long long parsed = 0ull;
  char *end = 0;

  if (is_empty(value) || out == 0) {
    return 0;
  }

  parsed = strtoull(value, &end, 10);
  if (end == value || *end != '\0') {
    return 0;
  }

  *out = (size_t)parsed;
  return (unsigned long long)*out == parsed;
}

static int parse_i32(const char *value, int *out) {
  long parsed = 0l;
  char *end = 0;

  if (is_empty(value) || out == 0) {
    return 0;
  }

  parsed = strtol(value, &end, 10);
  if (end == value || *end != '\0') {
    return 0;
  }

  *out = (int)parsed;
  return (long)*out == parsed;
}

static int import_surface_is_supported(int value) {
  return value >= (int)CORTEX_IMPORT_SURFACE_NONE &&
         value <= (int)CORTEX_IMPORT_SURFACE_QUARANTINE;
}

static int import_outcome_is_supported(int value) {
  return value >= (int)CORTEX_IMPORT_OUTCOME_ACCEPTED &&
         value <= (int)CORTEX_IMPORT_OUTCOME_REJECTED;
}

static int import_lifecycle_is_supported(int value) {
  return value >= (int)CORTEX_IMPORT_LIFECYCLE_STAGED &&
         value <= (int)CORTEX_IMPORT_LIFECYCLE_RETIRED;
}

static int import_error_is_supported(int value) {
  return value >= (int)CORTEX_IMPORT_OK &&
         value <= (int)CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED;
}

static int ara_shape_is_supported(int value) {
  return value == (int)CORTEX_ARA_SHAPE_TWO_COMPONENT ||
         value == (int)CORTEX_ARA_SHAPE_THREE_COMPONENT ||
         value == (int)CORTEX_ARA_SHAPE_LARGER_JUSTIFIED;
}

static int character_state_is_supported(int value) {
  return value >= (int)CORTEX_CHARACTER_DRAFT &&
         value <= (int)CORTEX_CHARACTER_ARCHIVED;
}

static int component_role_is_supported(int value) {
  return value >= (int)CORTEX_COMPONENT_ROLE_ACTIVE &&
         value <= (int)CORTEX_COMPONENT_ROLE_REASONING;
}

static int component_state_is_supported(int value) {
  return value >= (int)CORTEX_COMPONENT_STAGED &&
         value <= (int)CORTEX_COMPONENT_ARCHIVED;
}

static int subagent_state_is_supported(int value) {
  return value >= (int)CORTEX_SUBAGENT_DEFINED &&
         value <= (int)CORTEX_SUBAGENT_ARCHIVED;
}

static int read_next_record(FILE *file, char *line, size_t line_capacity) {
  do {
    if (fgets(line, (int)line_capacity, file) == 0) {
      return 0;
    }
    trim_line(line);
  } while (line[0] == '\0');

  return 1;
}

static cortex_persistence_error validate_snapshot_for_save(
  const cortex_runtime_state_snapshot *snapshot,
  cortex_persistence_result *result) {
  size_t index = 0u;

  if (snapshot == 0 ||
      snapshot->schema_version != CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION ||
      snapshot->audit_count > snapshot->audit_capacity ||
      snapshot->character_count > snapshot->character_capacity ||
      snapshot->component_count > snapshot->component_capacity ||
      snapshot->subagent_count > snapshot->subagent_capacity ||
      (snapshot->audit_count != 0u && snapshot->audits == 0) ||
      (snapshot->character_count != 0u && snapshot->characters == 0) ||
      (snapshot->component_count != 0u && snapshot->components == 0) ||
      (snapshot->subagent_count != 0u && snapshot->subagents == 0)) {
    return finish(result, CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT,
                  "snapshot_invalid",
                  "snapshot pointers and schema version must be valid before save",
                  0u, 0u);
  }

  for (index = 0u; index < snapshot->audit_count; ++index) {
    const cortex_persisted_import_audit *audit = &snapshot->audits[index];
    if (!fits_string(audit->registration_id, sizeof(audit->registration_id)) ||
        !fits_string(audit->source_repo, sizeof(audit->source_repo)) ||
        !fits_string(audit->source_path, sizeof(audit->source_path)) ||
        !fits_string(audit->source_commit, sizeof(audit->source_commit)) ||
        !fits_string(audit->descriptor_id, sizeof(audit->descriptor_id)) ||
        !fits_string(audit->rule_id, sizeof(audit->rule_id)) ||
        !fits_string(audit->message, sizeof(audit->message))) {
      return finish(result, CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED,
                    "audit_field_unsupported",
                    "audit fields must be portable for the deterministic text store",
                    index, index);
    }
  }

  for (index = 0u; index < snapshot->character_count; ++index) {
    const cortex_persisted_character_record *character = &snapshot->characters[index];
    if (!fits_string(character->character_id, sizeof(character->character_id)) ||
        !fits_string(character->owner_identity_ref, sizeof(character->owner_identity_ref))) {
      return finish(result, CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED,
                    "character_field_unsupported",
                    "character fields must be portable for the deterministic text store",
                    index, index);
    }
  }

  for (index = 0u; index < snapshot->component_count; ++index) {
    const cortex_persisted_component_record *component = &snapshot->components[index];
    if (!fits_string(component->component_id, sizeof(component->component_id)) ||
        !fits_string(component->owner_identity_ref, sizeof(component->owner_identity_ref))) {
      return finish(result, CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED,
                    "component_field_unsupported",
                    "component fields must be portable for the deterministic text store",
                    index, index);
    }
  }

  for (index = 0u; index < snapshot->subagent_count; ++index) {
    const cortex_persisted_subagent_record *subagent = &snapshot->subagents[index];
    if (!fits_string(subagent->subagent_id, sizeof(subagent->subagent_id)) ||
        !fits_string(subagent->definition_ref, sizeof(subagent->definition_ref))) {
      return finish(result, CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED,
                    "subagent_field_unsupported",
                    "subagent fields must be portable for the deterministic text store",
                    index, index);
    }
  }

  return CORTEX_PERSISTENCE_OK;
}

static int write_audit(FILE *file, const cortex_persisted_import_audit *audit) {
  return fprintf(file,
                 "A\t%u\t%u\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%u\t%u\n",
                 audit->schema_version,
                 audit->descriptor_version,
                 audit->registration_id,
                 audit->source_repo,
                 audit->source_path,
                 audit->source_commit,
                 audit->descriptor_id,
                 audit->rule_id,
                 audit->message,
                 (int)audit->surface,
                 (int)audit->outcome,
                 (int)audit->lifecycle_state,
                 (int)audit->import_error,
                 (unsigned int)audit->definition_only,
                 (unsigned int)audit->can_bind_as_subagent) > 0;
}

static int write_character(FILE *file, const cortex_persisted_character_record *character) {
  return fprintf(file, "C\t%s\t%s\t%d\t%d\t%zu\t%zu\n",
                 character->character_id,
                 character->owner_identity_ref,
                 (int)character->array_shape,
                 (int)character->lifecycle_state,
                 character->first_component_index,
                 character->component_count) > 0;
}

static int write_component(FILE *file, const cortex_persisted_component_record *component) {
  return fprintf(file, "M\t%s\t%s\t%u\t%d\t%d\t%u\t%u\n",
                 component->component_id,
                 component->owner_identity_ref,
                 component->slot_index,
                 (int)component->role,
                 (int)component->lifecycle_state,
                 component->subagent_limit,
                 component->live_or_queued_subagents) > 0;
}

static int write_subagent(FILE *file, const cortex_persisted_subagent_record *subagent) {
  return fprintf(file, "S\t%s\t%s\t%zu\t%d\t%u\n",
                 subagent->subagent_id,
                 subagent->definition_ref,
                 subagent->component_index,
                 (int)subagent->lifecycle_state,
                 (unsigned int)subagent->imported_definition) > 0;
}

static cortex_persistence_error load_audit(
  char **fields,
  size_t field_count,
  cortex_persisted_import_audit *audit) {
  uint32_t parsed_u32 = 0u;
  int parsed_i32 = 0;

  if (field_count != 16u || strcmp(fields[0], "A") != 0) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }

  if (!parse_u32(fields[1], &audit->schema_version) ||
      !parse_u32(fields[2], &audit->descriptor_version)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }

  if (audit->schema_version != CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION) {
    return CORTEX_ERR_PERSISTENCE_SCHEMA_MISMATCH;
  }

  if (!fits_string(fields[3], sizeof(audit->registration_id)) ||
      !fits_string(fields[4], sizeof(audit->source_repo)) ||
      !fits_string(fields[5], sizeof(audit->source_path)) ||
      !fits_string(fields[6], sizeof(audit->source_commit)) ||
      !fits_string(fields[7], sizeof(audit->descriptor_id)) ||
      !fits_string(fields[8], sizeof(audit->rule_id)) ||
      !fits_string(fields[9], sizeof(audit->message))) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }

  copy_string(audit->registration_id, sizeof(audit->registration_id), fields[3]);
  copy_string(audit->source_repo, sizeof(audit->source_repo), fields[4]);
  copy_string(audit->source_path, sizeof(audit->source_path), fields[5]);
  copy_string(audit->source_commit, sizeof(audit->source_commit), fields[6]);
  copy_string(audit->descriptor_id, sizeof(audit->descriptor_id), fields[7]);
  copy_string(audit->rule_id, sizeof(audit->rule_id), fields[8]);
  copy_string(audit->message, sizeof(audit->message), fields[9]);

  if (!parse_i32(fields[10], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!import_surface_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  audit->surface = (cortex_import_surface)parsed_i32;
  if (!parse_i32(fields[11], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!import_outcome_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  audit->outcome = (cortex_import_outcome)parsed_i32;
  if (!parse_i32(fields[12], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!import_lifecycle_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  audit->lifecycle_state = (cortex_import_lifecycle_state)parsed_i32;
  if (!parse_i32(fields[13], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!import_error_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  audit->import_error = (cortex_import_error)parsed_i32;
  if (!parse_u32(fields[14], &parsed_u32) || parsed_u32 > 1u) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  audit->definition_only = (uint8_t)parsed_u32;
  if (!parse_u32(fields[15], &parsed_u32) || parsed_u32 > 1u) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  audit->can_bind_as_subagent = (uint8_t)parsed_u32;
  return CORTEX_PERSISTENCE_OK;
}

static cortex_persistence_error load_character(
  char **fields,
  size_t field_count,
  cortex_persisted_character_record *character) {
  int parsed_i32 = 0;

  if (field_count != 7u || strcmp(fields[0], "C") != 0 ||
      !fits_string(fields[1], sizeof(character->character_id)) ||
      !fits_string(fields[2], sizeof(character->owner_identity_ref))) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }

  copy_string(character->character_id, sizeof(character->character_id), fields[1]);
  copy_string(character->owner_identity_ref, sizeof(character->owner_identity_ref), fields[2]);

  if (!parse_i32(fields[3], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!ara_shape_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  character->array_shape = (cortex_ara_shape)parsed_i32;
  if (!parse_i32(fields[4], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!character_state_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  character->lifecycle_state = (cortex_character_state)parsed_i32;
  if (!parse_size(fields[5], &character->first_component_index) ||
      !parse_size(fields[6], &character->component_count)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  return CORTEX_PERSISTENCE_OK;
}

static cortex_persistence_error load_component(
  char **fields,
  size_t field_count,
  cortex_persisted_component_record *component) {
  int parsed_i32 = 0;

  if (field_count != 8u || strcmp(fields[0], "M") != 0 ||
      !fits_string(fields[1], sizeof(component->component_id)) ||
      !fits_string(fields[2], sizeof(component->owner_identity_ref))) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }

  copy_string(component->component_id, sizeof(component->component_id), fields[1]);
  copy_string(component->owner_identity_ref, sizeof(component->owner_identity_ref), fields[2]);
  if (!parse_u32(fields[3], &component->slot_index) ||
      !parse_i32(fields[4], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!component_role_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  component->role = (cortex_component_role)parsed_i32;
  if (!parse_i32(fields[5], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!component_state_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  component->lifecycle_state = (cortex_component_state)parsed_i32;
  if (!parse_u32(fields[6], &component->subagent_limit) ||
      !parse_u32(fields[7], &component->live_or_queued_subagents)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  return CORTEX_PERSISTENCE_OK;
}

static cortex_persistence_error load_subagent(
  char **fields,
  size_t field_count,
  cortex_persisted_subagent_record *subagent) {
  int parsed_i32 = 0;
  uint32_t parsed_u32 = 0u;

  if (field_count != 6u || strcmp(fields[0], "S") != 0 ||
      !fits_string(fields[1], sizeof(subagent->subagent_id)) ||
      !fits_string(fields[2], sizeof(subagent->definition_ref))) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }

  copy_string(subagent->subagent_id, sizeof(subagent->subagent_id), fields[1]);
  copy_string(subagent->definition_ref, sizeof(subagent->definition_ref), fields[2]);
  if (!parse_size(fields[3], &subagent->component_index) ||
      !parse_i32(fields[4], &parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_PARSE;
  }
  if (!subagent_state_is_supported(parsed_i32)) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  subagent->lifecycle_state = (cortex_subagent_state)parsed_i32;
  if (!parse_u32(fields[5], &parsed_u32) || parsed_u32 > 1u) {
    return CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED;
  }
  subagent->imported_definition = (uint8_t)parsed_u32;
  return CORTEX_PERSISTENCE_OK;
}

uint32_t cortex_runtime_persistence_abi_version(void) {
  return (CORTEX_RUNTIME_PERSISTENCE_ABI_VERSION_MAJOR << 16) |
         (CORTEX_RUNTIME_PERSISTENCE_ABI_VERSION_MINOR << 8) |
         CORTEX_RUNTIME_PERSISTENCE_ABI_VERSION_PATCH;
}

uint32_t cortex_runtime_persistence_schema_version(void) {
  return CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION;
}

cortex_persistence_error cortex_catalog_ingest_batch(
  const cortex_catalog_registration *registrations,
  size_t registration_count,
  cortex_persisted_import_audit *audit_records,
  size_t audit_capacity,
  size_t *audit_count,
  cortex_persistence_result *result) {
  size_t index = 0u;
  cortex_import_mapping_result mapping;
  cortex_persistence_error store_error = CORTEX_PERSISTENCE_OK;

  clear_result(result);

  if (audit_count != 0) {
    *audit_count = 0u;
  }

  if ((registration_count != 0u && registrations == 0) ||
      (registration_count != 0u && audit_records == 0) ||
      audit_count == 0) {
    return finish(result, CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT,
                  "ingest_arguments_required",
                  "registrations, audit storage, and audit count output are required",
                  0u, 0u);
  }

  if (registration_count > audit_capacity) {
    return finish(result, CORTEX_ERR_PERSISTENCE_CAPACITY_EXCEEDED,
                  "audit_capacity_exceeded",
                  "audit output storage cannot hold every catalog registration result",
                  audit_capacity, 0u);
  }

  for (index = 0u; index < registration_count; ++index) {
    (void)cortex_import_map_registration(&registrations[index], &mapping);
    store_error = store_audit_record(&registrations[index], &mapping,
                                     &audit_records[index], index, result);
    if (store_error != CORTEX_PERSISTENCE_OK) {
      *audit_count = index;
      return store_error;
    }
  }

  *audit_count = registration_count;
  return finish(result, CORTEX_PERSISTENCE_OK,
                "catalog_batch_ingested",
                "catalog registrations mapped and durable audit records produced",
                0u, registration_count);
}

cortex_persistence_error cortex_runtime_capture_character(
  const cortex_character *character,
  const cortex_subagent_instance *subagents,
  size_t subagent_count,
  cortex_runtime_state_snapshot *snapshot,
  cortex_persistence_result *result) {
  size_t index = 0u;
  size_t first_component = 0u;
  size_t first_subagent = 0u;
  cortex_runtime_result runtime_result;

  clear_result(result);

  if (character == 0 || snapshot == 0 ||
      snapshot->schema_version != CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION ||
      character->components == 0 ||
      snapshot->audit_count > snapshot->audit_capacity ||
      snapshot->character_count > snapshot->character_capacity ||
      snapshot->component_count > snapshot->component_capacity ||
      snapshot->subagent_count > snapshot->subagent_capacity ||
      (subagent_count != 0u && subagents == 0)) {
    return finish(result, CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT,
                  "capture_arguments_required",
                  "character, components, and a schema-compatible snapshot are required",
                  0u, 0u);
  }

  if (cortex_character_validate(character, &runtime_result) != CORTEX_RUNTIME_OK) {
    return finish(result, CORTEX_ERR_PERSISTENCE_RUNTIME_REJECTED,
                  runtime_result.rule_id,
                  runtime_result.message,
                  runtime_result.failing_index,
                  0u);
  }

  if (snapshot->characters == 0 || snapshot->components == 0 ||
      snapshot->character_count >= snapshot->character_capacity ||
      character->component_count > snapshot->component_capacity - snapshot->component_count ||
      subagent_count > snapshot->subagent_capacity - snapshot->subagent_count ||
      (subagent_count != 0u && snapshot->subagents == 0)) {
    return finish(result, CORTEX_ERR_PERSISTENCE_CAPACITY_EXCEEDED,
                  "snapshot_capacity_exceeded",
                  "snapshot storage cannot hold the character, components, or subagents",
                  snapshot->character_count, 0u);
  }

  if (!fits_string(character->character_id, CORTEX_PERSISTED_ID_CAPACITY) ||
      !fits_string(character->owner_identity_ref, CORTEX_PERSISTED_ID_CAPACITY)) {
    return finish(result, CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED,
                  "character_field_unsupported",
                  "character id and owner fields must fit persisted storage",
                  0u, 0u);
  }

  for (index = 0u; index < character->component_count; ++index) {
    if (!fits_string(character->components[index].component_id, CORTEX_PERSISTED_ID_CAPACITY) ||
        !fits_string(character->components[index].owner_identity_ref, CORTEX_PERSISTED_ID_CAPACITY)) {
      return finish(result, CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED,
                    "component_field_unsupported",
                    "component id and owner fields must fit persisted storage",
                    index, 0u);
    }
  }

  for (index = 0u; index < subagent_count; ++index) {
    if (!fits_string(subagents[index].subagent_id, CORTEX_PERSISTED_ID_CAPACITY) ||
        !fits_string(subagents[index].definition_ref, CORTEX_PERSISTED_ID_CAPACITY)) {
      return finish(result, CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED,
                    "subagent_field_unsupported",
                    "subagent id and definition fields must fit persisted storage",
                    index, 0u);
    }
  }

  first_component = snapshot->component_count;
  first_subagent = snapshot->subagent_count;
  copy_string(snapshot->characters[snapshot->character_count].character_id,
              sizeof(snapshot->characters[snapshot->character_count].character_id),
              character->character_id);
  copy_string(snapshot->characters[snapshot->character_count].owner_identity_ref,
              sizeof(snapshot->characters[snapshot->character_count].owner_identity_ref),
              character->owner_identity_ref);
  snapshot->characters[snapshot->character_count].array_shape = character->array_shape;
  snapshot->characters[snapshot->character_count].lifecycle_state = character->lifecycle_state;
  snapshot->characters[snapshot->character_count].first_component_index = first_component;
  snapshot->characters[snapshot->character_count].component_count = character->component_count;

  for (index = 0u; index < character->component_count; ++index) {
    cortex_persisted_component_record *record = &snapshot->components[first_component + index];
    const cortex_ara_component *component = &character->components[index];
    copy_string(record->component_id, sizeof(record->component_id), component->component_id);
    copy_string(record->owner_identity_ref, sizeof(record->owner_identity_ref), component->owner_identity_ref);
    record->slot_index = component->slot_index;
    record->role = component->role;
    record->lifecycle_state = component->lifecycle_state;
    record->subagent_limit = component->subagent_limit;
    record->live_or_queued_subagents = component->live_or_queued_subagents;
  }

  for (index = 0u; index < subagent_count; ++index) {
    cortex_persisted_subagent_record *record = &snapshot->subagents[first_subagent + index];
    copy_string(record->subagent_id, sizeof(record->subagent_id), subagents[index].subagent_id);
    copy_string(record->definition_ref, sizeof(record->definition_ref), subagents[index].definition_ref);
    record->component_index = subagents[index].component_index;
    record->lifecycle_state = subagents[index].lifecycle_state;
    record->imported_definition = subagents[index].imported_definition;
  }

  snapshot->character_count += 1u;
  snapshot->component_count += character->component_count;
  snapshot->subagent_count += subagent_count;
  return finish(result, CORTEX_PERSISTENCE_OK,
                "character_state_captured",
                "character, component, and subagent runtime state copied into the durable snapshot",
                0u, 1u);
}

cortex_persistence_error cortex_runtime_state_save(
  const char *path,
  const cortex_runtime_state_snapshot *snapshot,
  cortex_persistence_result *result) {
  FILE *file = 0;
  size_t index = 0u;
  cortex_persistence_error validation = CORTEX_PERSISTENCE_OK;

  clear_result(result);

  if (is_empty(path)) {
    return finish(result, CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT,
                  "path_required",
                  "a state file path is required",
                  0u, 0u);
  }

  validation = validate_snapshot_for_save(snapshot, result);
  if (validation != CORTEX_PERSISTENCE_OK) {
    return validation;
  }

  file = open_file(path, "wb");
  if (file == 0) {
    return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                  "state_file_open_failed",
                  "state file could not be opened for writing",
                  0u, 0u);
  }

  if (fprintf(file, "%s\t%u\n", CORTEX_STATE_MAGIC, CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION) <= 0 ||
      fprintf(file, "COUNTS\t%zu\t%zu\t%zu\t%zu\n",
              snapshot->audit_count,
              snapshot->character_count,
              snapshot->component_count,
              snapshot->subagent_count) <= 0) {
    (void)fclose(file);
    return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                  "state_file_write_failed",
                  "state file header could not be written",
                  0u, 0u);
  }

  for (index = 0u; index < snapshot->audit_count; ++index) {
    if (!write_audit(file, &snapshot->audits[index])) {
      (void)fclose(file);
      return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                    "audit_write_failed",
                    "audit record could not be written",
                    index, index);
    }
  }

  for (index = 0u; index < snapshot->character_count; ++index) {
    if (!write_character(file, &snapshot->characters[index])) {
      (void)fclose(file);
      return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                    "character_write_failed",
                    "character record could not be written",
                    index, snapshot->audit_count + index);
    }
  }

  for (index = 0u; index < snapshot->component_count; ++index) {
    if (!write_component(file, &snapshot->components[index])) {
      (void)fclose(file);
      return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                    "component_write_failed",
                    "component record could not be written",
                    index, snapshot->audit_count + snapshot->character_count + index);
    }
  }

  for (index = 0u; index < snapshot->subagent_count; ++index) {
    if (!write_subagent(file, &snapshot->subagents[index])) {
      (void)fclose(file);
      return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                    "subagent_write_failed",
                    "subagent record could not be written",
                    index,
                    snapshot->audit_count + snapshot->character_count +
                      snapshot->component_count + index);
    }
  }

  if (fprintf(file, "END\n") <= 0 || fclose(file) != 0) {
    return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                  "state_file_close_failed",
                  "state file could not be finalized",
                  0u, 0u);
  }

  return finish(result, CORTEX_PERSISTENCE_OK,
                "runtime_state_saved",
                "runtime state snapshot persisted",
                0u,
                snapshot->audit_count + snapshot->character_count +
                  snapshot->component_count + snapshot->subagent_count);
}

cortex_persistence_error cortex_runtime_state_load(
  const char *path,
  cortex_runtime_state_snapshot *snapshot,
  cortex_persistence_result *result) {
  FILE *file = 0;
  char line[CORTEX_STATE_LINE_CAPACITY];
  char *fields[CORTEX_STATE_MAX_FIELDS];
  size_t field_count = 0u;
  uint32_t schema_version = 0u;
  size_t expected_audits = 0u;
  size_t expected_characters = 0u;
  size_t expected_components = 0u;
  size_t expected_subagents = 0u;
  size_t index = 0u;

  clear_result(result);

  if (is_empty(path) || snapshot == 0 ||
      snapshot->schema_version != CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION) {
    return finish(result, CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT,
                  "load_arguments_required",
                  "a state path and schema-compatible snapshot are required",
                  0u, 0u);
  }

  file = open_file(path, "rb");
  if (file == 0) {
    return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                  "state_file_open_failed",
                  "state file could not be opened for reading",
                  0u, 0u);
  }

  if (!read_next_record(file, line, sizeof(line))) {
    (void)fclose(file);
    return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                  "state_header_missing",
                  "state file header is missing",
                  0u, 0u);
  }

  field_count = split_fields(line, fields, CORTEX_STATE_MAX_FIELDS);
  if (field_count != 2u || strcmp(fields[0], CORTEX_STATE_MAGIC) != 0 ||
      !parse_u32(fields[1], &schema_version)) {
    (void)fclose(file);
    return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                  "state_header_invalid",
                  "state file header is invalid",
                  0u, 0u);
  }

  if (schema_version != CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION) {
    (void)fclose(file);
    return finish(result, CORTEX_ERR_PERSISTENCE_SCHEMA_MISMATCH,
                  "schema_version_mismatch",
                  "state file schema version is not supported by this runtime",
                  0u, 0u);
  }

  if (!read_next_record(file, line, sizeof(line))) {
    (void)fclose(file);
    return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                  "state_counts_missing",
                  "state file counts are missing",
                  0u, 0u);
  }

  field_count = split_fields(line, fields, CORTEX_STATE_MAX_FIELDS);
  if (field_count != 5u || strcmp(fields[0], "COUNTS") != 0 ||
      !parse_size(fields[1], &expected_audits) ||
      !parse_size(fields[2], &expected_characters) ||
      !parse_size(fields[3], &expected_components) ||
      !parse_size(fields[4], &expected_subagents)) {
    (void)fclose(file);
    return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                  "state_counts_invalid",
                  "state file counts are invalid",
                  0u, 0u);
  }

  if (expected_audits > snapshot->audit_capacity ||
      expected_characters > snapshot->character_capacity ||
      expected_components > snapshot->component_capacity ||
      expected_subagents > snapshot->subagent_capacity ||
      (expected_audits != 0u && snapshot->audits == 0) ||
      (expected_characters != 0u && snapshot->characters == 0) ||
      (expected_components != 0u && snapshot->components == 0) ||
      (expected_subagents != 0u && snapshot->subagents == 0)) {
    (void)fclose(file);
    return finish(result, CORTEX_ERR_PERSISTENCE_CAPACITY_EXCEEDED,
                  "load_capacity_exceeded",
                  "snapshot storage cannot hold the state file contents",
                  0u, 0u);
  }

  for (index = 0u; index < expected_audits; ++index) {
    cortex_persistence_error load_error = CORTEX_PERSISTENCE_OK;
    if (!read_next_record(file, line, sizeof(line))) {
      (void)fclose(file);
      return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                    "audit_record_missing",
                    "state file ended before all audit records were read",
                    index, index);
    }
    field_count = split_fields(line, fields, CORTEX_STATE_MAX_FIELDS);
    load_error = load_audit(fields, field_count, &snapshot->audits[index]);
    if (load_error != CORTEX_PERSISTENCE_OK) {
      (void)fclose(file);
      return finish(result, load_error,
                    "audit_record_invalid",
                    "audit record could not be parsed",
                    index, index);
    }
  }

  for (index = 0u; index < expected_characters; ++index) {
    cortex_persistence_error load_error = CORTEX_PERSISTENCE_OK;
    if (!read_next_record(file, line, sizeof(line))) {
      (void)fclose(file);
      return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                    "character_record_missing",
                    "state file ended before all character records were read",
                    index, expected_audits + index);
    }
    field_count = split_fields(line, fields, CORTEX_STATE_MAX_FIELDS);
    load_error = load_character(fields, field_count, &snapshot->characters[index]);
    if (load_error != CORTEX_PERSISTENCE_OK) {
      (void)fclose(file);
      return finish(result, load_error,
                    "character_record_invalid",
                    "character record could not be parsed",
                    index, expected_audits + index);
    }
  }

  for (index = 0u; index < expected_components; ++index) {
    cortex_persistence_error load_error = CORTEX_PERSISTENCE_OK;
    if (!read_next_record(file, line, sizeof(line))) {
      (void)fclose(file);
      return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                    "component_record_missing",
                    "state file ended before all component records were read",
                    index, expected_audits + expected_characters + index);
    }
    field_count = split_fields(line, fields, CORTEX_STATE_MAX_FIELDS);
    load_error = load_component(fields, field_count, &snapshot->components[index]);
    if (load_error != CORTEX_PERSISTENCE_OK) {
      (void)fclose(file);
      return finish(result, load_error,
                    "component_record_invalid",
                    "component record could not be parsed",
                    index, expected_audits + expected_characters + index);
    }
  }

  for (index = 0u; index < expected_subagents; ++index) {
    cortex_persistence_error load_error = CORTEX_PERSISTENCE_OK;
    if (!read_next_record(file, line, sizeof(line))) {
      (void)fclose(file);
      return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                    "subagent_record_missing",
                    "state file ended before all subagent records were read",
                    index, expected_audits + expected_characters + expected_components + index);
    }
    field_count = split_fields(line, fields, CORTEX_STATE_MAX_FIELDS);
    load_error = load_subagent(fields, field_count, &snapshot->subagents[index]);
    if (load_error != CORTEX_PERSISTENCE_OK) {
      (void)fclose(file);
      return finish(result, load_error,
                    "subagent_record_invalid",
                    "subagent record could not be parsed",
                    index, expected_audits + expected_characters + expected_components + index);
    }
  }

  if (!read_next_record(file, line, sizeof(line)) || strcmp(line, "END") != 0) {
    (void)fclose(file);
    return finish(result, CORTEX_ERR_PERSISTENCE_PARSE,
                  "state_end_missing",
                  "state file terminator is missing",
                  0u, expected_audits + expected_characters + expected_components + expected_subagents);
  }

  if (fclose(file) != 0) {
    return finish(result, CORTEX_ERR_PERSISTENCE_IO,
                  "state_file_close_failed",
                  "state file could not be closed after read",
                  0u, 0u);
  }

  snapshot->audit_count = expected_audits;
  snapshot->character_count = expected_characters;
  snapshot->component_count = expected_components;
  snapshot->subagent_count = expected_subagents;

  return finish(result, CORTEX_PERSISTENCE_OK,
                "runtime_state_loaded",
                "runtime state snapshot loaded",
                0u,
                expected_audits + expected_characters + expected_components + expected_subagents);
}

const char *cortex_persistence_error_name(cortex_persistence_error error) {
  switch (error) {
    case CORTEX_PERSISTENCE_OK:
      return "CORTEX_PERSISTENCE_OK";
    case CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT:
      return "CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT";
    case CORTEX_ERR_PERSISTENCE_IO:
      return "CORTEX_ERR_PERSISTENCE_IO";
    case CORTEX_ERR_PERSISTENCE_CAPACITY_EXCEEDED:
      return "CORTEX_ERR_PERSISTENCE_CAPACITY_EXCEEDED";
    case CORTEX_ERR_PERSISTENCE_SCHEMA_MISMATCH:
      return "CORTEX_ERR_PERSISTENCE_SCHEMA_MISMATCH";
    case CORTEX_ERR_PERSISTENCE_PARSE:
      return "CORTEX_ERR_PERSISTENCE_PARSE";
    case CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED:
      return "CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED";
    case CORTEX_ERR_PERSISTENCE_RUNTIME_REJECTED:
      return "CORTEX_ERR_PERSISTENCE_RUNTIME_REJECTED";
    default:
      return "CORTEX_ERR_PERSISTENCE_UNKNOWN";
  }
}
