#include "cortex/runtime_persistence.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CORTEX_HOST_MAX_REGISTRATIONS 32u
#define CORTEX_HOST_MAX_HOSTS 8u
#define CORTEX_HOST_MAX_ROLES 8u
#define CORTEX_HOST_TEXT_CAPACITY 128u
#define CORTEX_HOST_LIST_CAPACITY 256u
#define CORTEX_HOST_LINE_CAPACITY 1024u
#define CORTEX_HOST_SNAPSHOT_AUDITS 64u
#define CORTEX_HOST_SNAPSHOT_CHARACTERS 8u
#define CORTEX_HOST_SNAPSHOT_COMPONENTS 24u
#define CORTEX_HOST_SNAPSHOT_SUBAGENTS 64u

enum cortex_host_exit {
  CORTEX_HOST_EXIT_OK = 0,
  CORTEX_HOST_EXIT_USAGE = 2,
  CORTEX_HOST_EXIT_IO = 3,
  CORTEX_HOST_EXIT_STATE = 4,
  CORTEX_HOST_EXIT_RUNTIME = 5
};

typedef struct cortex_host_catalog_row {
  cortex_catalog_registration registration;
  char registration_id[CORTEX_HOST_TEXT_CAPACITY];
  char runtime_class[CORTEX_HOST_TEXT_CAPACITY];
  char host_list[CORTEX_HOST_LIST_CAPACITY];
  char role_list[CORTEX_HOST_LIST_CAPACITY];
  char requested_persistent_class[CORTEX_HOST_TEXT_CAPACITY];
  char host_values[CORTEX_HOST_MAX_HOSTS][CORTEX_HOST_TEXT_CAPACITY];
  const char *host_targets[CORTEX_HOST_MAX_HOSTS];
  char role_values[CORTEX_HOST_MAX_ROLES][CORTEX_HOST_TEXT_CAPACITY];
  const char *component_roles[CORTEX_HOST_MAX_ROLES];
} cortex_host_catalog_row;

static int is_empty(const char *value) {
  return value == 0 || value[0] == '\0';
}

static void trim(char *value) {
  char *start = value;
  size_t length = 0u;

  if (value == 0) {
    return;
  }

  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
    ++start;
  }

  if (start != value) {
    memmove(value, start, strlen(start) + 1u);
  }

  length = strlen(value);
  while (length > 0u &&
         (value[length - 1u] == ' ' ||
          value[length - 1u] == '\t' ||
          value[length - 1u] == '\r' ||
          value[length - 1u] == '\n')) {
    value[length - 1u] = '\0';
    --length;
  }
}

static void strip_line_endings(char *value) {
  size_t length = 0u;

  if (value == 0) {
    return;
  }

  length = strlen(value);
  while (length > 0u &&
         (value[length - 1u] == '\r' || value[length - 1u] == '\n')) {
    value[length - 1u] = '\0';
    --length;
  }
}

static int copy_text(char *target, size_t capacity, const char *source) {
  size_t length = source == 0 ? 0u : strlen(source);

  if (target == 0 || capacity == 0u || length >= capacity) {
    return 0;
  }

  if (source == 0) {
    target[0] = '\0';
  } else {
    memcpy(target, source, length + 1u);
  }

  return 1;
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

static int split_tabs(char *line, char *fields[], size_t field_capacity) {
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

  return (int)count;
}

static int parse_csv(
  char *text,
  char values[][CORTEX_HOST_TEXT_CAPACITY],
  const char *pointers[],
  size_t capacity,
  size_t *count) {
  size_t item_count = 0u;
  char *cursor = text;

  *count = 0u;
  trim(text);
  if (is_empty(text)) {
    return 1;
  }

  while (cursor != 0 && *cursor != '\0') {
    char *next = strchr(cursor, ',');
    if (item_count >= capacity) {
      return 0;
    }
    if (next != 0) {
      *next = '\0';
      ++next;
    }
    trim(cursor);
    if (is_empty(cursor) ||
        !copy_text(values[item_count], CORTEX_HOST_TEXT_CAPACITY, cursor)) {
      return 0;
    }
    pointers[item_count] = values[item_count];
    ++item_count;
    cursor = next;
  }

  *count = item_count;
  return 1;
}

static cortex_runtime_state_snapshot make_snapshot(
  cortex_persisted_import_audit *audits,
  size_t audit_capacity,
  cortex_persisted_character_record *characters,
  size_t character_capacity,
  cortex_persisted_component_record *components,
  size_t component_capacity,
  cortex_persisted_subagent_record *subagents,
  size_t subagent_capacity) {
  cortex_runtime_state_snapshot snapshot;

  memset(&snapshot, 0, sizeof(snapshot));
  snapshot.schema_version = CORTEX_RUNTIME_PERSISTENCE_SCHEMA_VERSION;
  snapshot.audits = audits;
  snapshot.audit_capacity = audit_capacity;
  snapshot.characters = characters;
  snapshot.character_capacity = character_capacity;
  snapshot.components = components;
  snapshot.component_capacity = component_capacity;
  snapshot.subagents = subagents;
  snapshot.subagent_capacity = subagent_capacity;
  return snapshot;
}

static int exit_for_persistence(cortex_persistence_error error) {
  if (error == CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT) {
    return CORTEX_HOST_EXIT_USAGE;
  }
  if (error == CORTEX_ERR_PERSISTENCE_IO) {
    return CORTEX_HOST_EXIT_IO;
  }
  if (error == CORTEX_ERR_PERSISTENCE_RUNTIME_REJECTED) {
    return CORTEX_HOST_EXIT_RUNTIME;
  }
  return CORTEX_HOST_EXIT_STATE;
}

static int report_persistence_error(
  const char *operation,
  cortex_persistence_error error,
  const cortex_persistence_result *result) {
  fprintf(stderr, "%s failed: %s", operation, cortex_persistence_error_name(error));
  if (result != 0 && result->rule_id != 0) {
    fprintf(stderr, " rule=%s", result->rule_id);
  }
  if (result != 0 && result->message != 0) {
    fprintf(stderr, " message=\"%s\"", result->message);
  }
  if (result != 0) {
    fprintf(stderr, " index=%zu processed=%zu", result->failing_index, result->processed_count);
  }
  fputc('\n', stderr);
  return exit_for_persistence(error);
}

static int load_snapshot(const char *path, cortex_runtime_state_snapshot *snapshot) {
  cortex_persistence_result result;
  cortex_persistence_error error = cortex_runtime_state_load(path, snapshot, &result);

  if (error != CORTEX_PERSISTENCE_OK) {
    return report_persistence_error("load", error, &result);
  }

  return CORTEX_HOST_EXIT_OK;
}

static cortex_character make_demo_character(cortex_ara_component components[2]) {
  cortex_character character;

  components[0].component_id = "component-active";
  components[0].slot_index = 0u;
  components[0].role = CORTEX_COMPONENT_ROLE_ACTIVE;
  components[0].owner_identity_ref = "symbiote-primary";
  components[0].lifecycle_state = CORTEX_COMPONENT_ACTIVE;
  components[0].subagent_limit = CORTEX_DEFAULT_SUBAGENT_LIMIT;
  components[0].live_or_queued_subagents = 0u;

  components[1].component_id = "component-memory";
  components[1].slot_index = 1u;
  components[1].role = CORTEX_COMPONENT_ROLE_MEMORY;
  components[1].owner_identity_ref = "curator-memory";
  components[1].lifecycle_state = CORTEX_COMPONENT_READY;
  components[1].subagent_limit = CORTEX_DEFAULT_SUBAGENT_LIMIT;
  components[1].live_or_queued_subagents = 0u;

  character.character_id = "character-alpha";
  character.owner_identity_ref = "symbiote-primary";
  character.array_shape = CORTEX_ARA_SHAPE_TWO_COMPONENT;
  character.lifecycle_state = CORTEX_CHARACTER_ACTIVE;
  character.components = components;
  character.component_count = 2u;
  return character;
}

static cortex_catalog_registration make_demo_registration(void) {
  static const char *host_targets[] = {"codex", "symbiosis-native"};
  static const char *roles[] = {"active"};
  cortex_catalog_registration registration;

  memset(&registration, 0, sizeof(registration));
  registration.registration_id = "host-demo-helper";
  registration.source_repo = "JKhyro/SYMBIOSIS";
  registration.source_path = ".codex/agents/host-demo.toml";
  registration.source_commit = "5f855c11f9117541da31e7274b738cc396d4d3c7";
  registration.source_manifest_ref = "khyron-subagent";
  registration.runtime_class = "imported-helper";
  registration.canonical_label = "Host demo helper";
  registration.compatibility_version = "1";
  registration.host_targets = host_targets;
  registration.host_target_count = 2u;
  registration.compatible_component_roles = roles;
  registration.compatible_component_role_count = 1u;
  return registration;
}

static void print_usage(void) {
  puts("cortex_host commands:");
  puts("  version");
  puts("  demo <state-file>");
  puts("  ingest <catalog-tsv> <state-file>");
  puts("  inspect <state-file>");
  puts("  audits <state-file>");
  puts("");
  puts("catalog-tsv columns: id, runtime_class, host_targets_csv, component_roles_csv, requested_persistent_class");
}

static int command_version(void) {
  printf("cortex_host abi=%u mapper_abi=%u character_abi=%u persistence_abi=%u schema=%u\n",
         1u,
         cortex_imported_catalog_mapper_abi_version(),
         cortex_character_runtime_abi_version(),
         cortex_runtime_persistence_abi_version(),
         cortex_runtime_persistence_schema_version());
  return CORTEX_HOST_EXIT_OK;
}

static int command_demo(const char *state_path) {
  cortex_catalog_registration registration = make_demo_registration();
  cortex_import_mapping_result mapping;
  cortex_ara_component components[2];
  cortex_character character = make_demo_character(components);
  cortex_subagent_instance subagent;
  cortex_runtime_result runtime_result;
  cortex_persisted_import_audit audits[2];
  cortex_persisted_character_record characters[1];
  cortex_persisted_component_record persisted_components[2];
  cortex_persisted_subagent_record persisted_subagents[1];
  cortex_runtime_state_snapshot snapshot;
  cortex_persistence_result persistence_result;
  cortex_persistence_error persistence_error = CORTEX_PERSISTENCE_OK;
  size_t audit_count = 0u;

  memset(audits, 0, sizeof(audits));
  memset(characters, 0, sizeof(characters));
  memset(persisted_components, 0, sizeof(persisted_components));
  memset(persisted_subagents, 0, sizeof(persisted_subagents));
  memset(&subagent, 0, sizeof(subagent));

  subagent.subagent_id = "subagent-host-demo";
  subagent.lifecycle_state = CORTEX_SUBAGENT_DEFINED;
  if (cortex_import_map_registration(&registration, &mapping) != CORTEX_IMPORT_OK) {
    fprintf(stderr, "demo mapping failed: %s\n", cortex_import_error_name(mapping.error));
    return CORTEX_HOST_EXIT_STATE;
  }

  if (cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &runtime_result) !=
      CORTEX_RUNTIME_OK) {
    fprintf(stderr, "demo bind failed: %s rule=%s\n",
            cortex_runtime_error_name(runtime_result.error),
            runtime_result.rule_id);
    return CORTEX_HOST_EXIT_RUNTIME;
  }

  if (cortex_subagent_apply_verb_for_component(&character, 0u, &subagent,
                                               CORTEX_VERB_CONFIRM_SUBAGENT_SPAWN,
                                               &runtime_result) != CORTEX_RUNTIME_OK) {
    fprintf(stderr, "demo spawn failed: %s rule=%s\n",
            cortex_runtime_error_name(runtime_result.error),
            runtime_result.rule_id);
    return CORTEX_HOST_EXIT_RUNTIME;
  }

  snapshot = make_snapshot(audits, 2u, characters, 1u, persisted_components, 2u,
                           persisted_subagents, 1u);
  persistence_error = cortex_catalog_ingest_batch(&registration, 1u, audits, 2u,
                                                  &audit_count, &persistence_result);
  if (persistence_error != CORTEX_PERSISTENCE_OK) {
    return report_persistence_error("demo ingest", persistence_error, &persistence_result);
  }
  snapshot.audit_count = audit_count;

  persistence_error = cortex_runtime_capture_character(&character, &subagent, 1u,
                                                       &snapshot, &persistence_result);
  if (persistence_error != CORTEX_PERSISTENCE_OK) {
    return report_persistence_error("demo capture", persistence_error, &persistence_result);
  }

  persistence_error = cortex_runtime_state_save(state_path, &snapshot, &persistence_result);
  if (persistence_error != CORTEX_PERSISTENCE_OK) {
    return report_persistence_error("demo save", persistence_error, &persistence_result);
  }

  printf("saved %s audits=%zu characters=%zu components=%zu subagents=%zu\n",
         state_path,
         snapshot.audit_count,
         snapshot.character_count,
         snapshot.component_count,
         snapshot.subagent_count);
  return CORTEX_HOST_EXIT_OK;
}

static int parse_catalog_line(
  char *line,
  cortex_host_catalog_row *row,
  const char *source_path,
  size_t line_number) {
  char *fields[5];
  size_t host_count = 0u;
  size_t role_count = 0u;

  if (split_tabs(line, fields, 5u) != 5) {
    fprintf(stderr, "invalid catalog line %zu: expected 5 tab-separated fields\n", line_number);
    return 0;
  }

  trim(fields[0]);
  trim(fields[1]);
  trim(fields[2]);
  trim(fields[3]);
  trim(fields[4]);
  if (is_empty(fields[0]) || is_empty(fields[1]) || is_empty(fields[2])) {
    fprintf(stderr, "invalid catalog line %zu: id, runtime_class, and host targets are required\n",
            line_number);
    return 0;
  }

  memset(row, 0, sizeof(*row));
  if (!copy_text(row->registration_id, sizeof(row->registration_id), fields[0]) ||
      !copy_text(row->runtime_class, sizeof(row->runtime_class), fields[1]) ||
      !copy_text(row->host_list, sizeof(row->host_list), fields[2]) ||
      !copy_text(row->role_list, sizeof(row->role_list), fields[3]) ||
      !copy_text(row->requested_persistent_class,
                 sizeof(row->requested_persistent_class), fields[4])) {
    fprintf(stderr, "invalid catalog line %zu: field exceeds host parser capacity\n", line_number);
    return 0;
  }

  if (!parse_csv(row->host_list, row->host_values, row->host_targets,
                 CORTEX_HOST_MAX_HOSTS, &host_count) ||
      !parse_csv(row->role_list, row->role_values, row->component_roles,
                 CORTEX_HOST_MAX_ROLES, &role_count)) {
    fprintf(stderr, "invalid catalog line %zu: host or role list is malformed\n", line_number);
    return 0;
  }

  row->registration.registration_id = row->registration_id;
  row->registration.source_repo = "local-cortex-host";
  row->registration.source_path = source_path;
  row->registration.source_commit = "working-tree";
  row->registration.source_manifest_ref = "cortex-host-tsv";
  row->registration.runtime_class = row->runtime_class;
  row->registration.canonical_label = row->registration_id;
  row->registration.compatibility_version = "1";
  row->registration.host_targets = row->host_targets;
  row->registration.host_target_count = host_count;
  row->registration.compatible_component_roles = row->component_roles;
  row->registration.compatible_component_role_count = role_count;
  row->registration.requested_persistent_class =
    is_empty(row->requested_persistent_class) ? 0 : row->requested_persistent_class;
  return 1;
}

static int load_catalog_file(
  const char *catalog_path,
  cortex_host_catalog_row rows[],
  size_t row_capacity,
  size_t *row_count) {
  FILE *file = open_file(catalog_path, "rb");
  char line[CORTEX_HOST_LINE_CAPACITY];
  size_t count = 0u;
  size_t line_number = 0u;

  *row_count = 0u;
  if (file == 0) {
    fprintf(stderr, "catalog open failed: %s\n", catalog_path);
    return 0;
  }

  while (fgets(line, sizeof(line), file) != 0) {
    ++line_number;
    strip_line_endings(line);
    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }
    if (count >= row_capacity) {
      fprintf(stderr, "catalog capacity exceeded at line %zu\n", line_number);
      (void)fclose(file);
      return 0;
    }
    if (!parse_catalog_line(line, &rows[count], catalog_path, line_number)) {
      (void)fclose(file);
      return 0;
    }
    ++count;
  }

  if (ferror(file)) {
    fprintf(stderr, "catalog read failed: %s\n", catalog_path);
    (void)fclose(file);
    return 0;
  }

  (void)fclose(file);
  if (count == 0u) {
    fprintf(stderr, "catalog has no registrations: %s\n", catalog_path);
    return 0;
  }

  *row_count = count;
  return 1;
}

static int command_ingest(const char *catalog_path, const char *state_path) {
  cortex_host_catalog_row rows[CORTEX_HOST_MAX_REGISTRATIONS];
  cortex_catalog_registration registrations[CORTEX_HOST_MAX_REGISTRATIONS];
  cortex_persisted_import_audit audits[CORTEX_HOST_MAX_REGISTRATIONS];
  cortex_runtime_state_snapshot snapshot;
  cortex_persistence_result result;
  cortex_persistence_error error = CORTEX_PERSISTENCE_OK;
  size_t row_count = 0u;
  size_t audit_count = 0u;
  size_t index = 0u;

  memset(rows, 0, sizeof(rows));
  memset(registrations, 0, sizeof(registrations));
  memset(audits, 0, sizeof(audits));
  if (!load_catalog_file(catalog_path, rows, CORTEX_HOST_MAX_REGISTRATIONS, &row_count)) {
    return CORTEX_HOST_EXIT_USAGE;
  }

  for (index = 0u; index < row_count; ++index) {
    registrations[index] = rows[index].registration;
  }

  error = cortex_catalog_ingest_batch(registrations, row_count, audits,
                                      CORTEX_HOST_MAX_REGISTRATIONS,
                                      &audit_count, &result);
  if (error != CORTEX_PERSISTENCE_OK) {
    return report_persistence_error("ingest", error, &result);
  }

  snapshot = make_snapshot(audits, CORTEX_HOST_MAX_REGISTRATIONS, 0, 0u, 0, 0u, 0, 0u);
  snapshot.audit_count = audit_count;
  error = cortex_runtime_state_save(state_path, &snapshot, &result);
  if (error != CORTEX_PERSISTENCE_OK) {
    return report_persistence_error("ingest save", error, &result);
  }

  printf("ingested %zu registrations to %s\n", audit_count, state_path);
  for (index = 0u; index < audit_count; ++index) {
    printf("audit[%zu] id=%s outcome=%s error=%s surface=%s\n",
           index,
           audits[index].registration_id,
           cortex_import_outcome_name(audits[index].outcome),
           cortex_import_error_name(audits[index].import_error),
           cortex_import_surface_name(audits[index].surface));
  }
  return CORTEX_HOST_EXIT_OK;
}

static int command_inspect(const char *state_path, int audits_only) {
  cortex_persisted_import_audit audits[CORTEX_HOST_SNAPSHOT_AUDITS];
  cortex_persisted_character_record characters[CORTEX_HOST_SNAPSHOT_CHARACTERS];
  cortex_persisted_component_record components[CORTEX_HOST_SNAPSHOT_COMPONENTS];
  cortex_persisted_subagent_record subagents[CORTEX_HOST_SNAPSHOT_SUBAGENTS];
  cortex_runtime_state_snapshot snapshot;
  int load_exit = CORTEX_HOST_EXIT_OK;
  size_t index = 0u;

  memset(audits, 0, sizeof(audits));
  memset(characters, 0, sizeof(characters));
  memset(components, 0, sizeof(components));
  memset(subagents, 0, sizeof(subagents));
  snapshot = make_snapshot(audits, CORTEX_HOST_SNAPSHOT_AUDITS,
                           characters, CORTEX_HOST_SNAPSHOT_CHARACTERS,
                           components, CORTEX_HOST_SNAPSHOT_COMPONENTS,
                           subagents, CORTEX_HOST_SNAPSHOT_SUBAGENTS);

  load_exit = load_snapshot(state_path, &snapshot);
  if (load_exit != CORTEX_HOST_EXIT_OK) {
    return load_exit;
  }

  printf("state %s schema=%u audits=%zu characters=%zu components=%zu subagents=%zu\n",
         state_path,
         snapshot.schema_version,
         snapshot.audit_count,
         snapshot.character_count,
         snapshot.component_count,
         snapshot.subagent_count);
  for (index = 0u; index < snapshot.audit_count; ++index) {
    printf("audit[%zu] id=%s outcome=%s error=%s surface=%s bindable=%u rule=%s\n",
           index,
           snapshot.audits[index].registration_id,
           cortex_import_outcome_name(snapshot.audits[index].outcome),
           cortex_import_error_name(snapshot.audits[index].import_error),
           cortex_import_surface_name(snapshot.audits[index].surface),
           (unsigned int)snapshot.audits[index].can_bind_as_subagent,
           snapshot.audits[index].rule_id);
  }

  if (audits_only) {
    return CORTEX_HOST_EXIT_OK;
  }

  for (index = 0u; index < snapshot.character_count; ++index) {
    printf("character[%zu] id=%s owner=%s state=%s components=%zu first_component=%zu\n",
           index,
           snapshot.characters[index].character_id,
           snapshot.characters[index].owner_identity_ref,
           cortex_character_state_name(snapshot.characters[index].lifecycle_state),
           snapshot.characters[index].component_count,
           snapshot.characters[index].first_component_index);
  }
  for (index = 0u; index < snapshot.component_count; ++index) {
    printf("component[%zu] id=%s role=%d state=%s live_or_queued=%u limit=%u\n",
           index,
           snapshot.components[index].component_id,
           (int)snapshot.components[index].role,
           cortex_component_state_name(snapshot.components[index].lifecycle_state),
           snapshot.components[index].live_or_queued_subagents,
           snapshot.components[index].subagent_limit);
  }
  for (index = 0u; index < snapshot.subagent_count; ++index) {
    printf("subagent[%zu] id=%s definition=%s component=%zu state=%s imported=%u\n",
           index,
           snapshot.subagents[index].subagent_id,
           snapshot.subagents[index].definition_ref,
           snapshot.subagents[index].component_index,
           cortex_subagent_state_name(snapshot.subagents[index].lifecycle_state),
           (unsigned int)snapshot.subagents[index].imported_definition);
  }
  return CORTEX_HOST_EXIT_OK;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return CORTEX_HOST_EXIT_USAGE;
  }

  if (strcmp(argv[1], "version") == 0) {
    return command_version();
  }

  if (strcmp(argv[1], "demo") == 0 && argc == 3) {
    return command_demo(argv[2]);
  }

  if (strcmp(argv[1], "ingest") == 0 && argc == 4) {
    return command_ingest(argv[2], argv[3]);
  }

  if (strcmp(argv[1], "inspect") == 0 && argc == 3) {
    return command_inspect(argv[2], 0);
  }

  if (strcmp(argv[1], "audits") == 0 && argc == 3) {
    return command_inspect(argv[2], 1);
  }

  print_usage();
  return CORTEX_HOST_EXIT_USAGE;
}
