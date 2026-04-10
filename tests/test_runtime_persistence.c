#include "cortex/runtime_persistence.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(condition) \
  do { \
    if (!(condition)) { \
      fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
      ++failures; \
    } \
  } while (0)

static cortex_catalog_registration base_registration(const char *id) {
  static const char *host_targets[] = {"codex", "symbiosis-native"};
  cortex_catalog_registration registration;

  memset(&registration, 0, sizeof(registration));
  registration.registration_id = id;
  registration.source_repo = "JKhyro/SYMBIOSIS";
  registration.source_path = ".codex/agents/helper.toml";
  registration.source_commit = "5f855c11f9117541da31e7274b738cc396d4d3c7";
  registration.source_manifest_ref = "khyron-subagent";
  registration.canonical_label = "Imported helper";
  registration.compatibility_version = "1";
  registration.host_targets = host_targets;
  registration.host_target_count = 2u;
  return registration;
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

static cortex_character make_two_component_character(cortex_ara_component components[2]) {
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

static void ingests_batch_and_records_all_mapper_outcomes(void) {
  static const char *roles[] = {"active"};
  static const char *mixed_hosts[] = {"codex", "root-shell"};
  cortex_catalog_registration registrations[4];
  cortex_persisted_import_audit audits[4];
  size_t audit_count = 0u;
  cortex_persistence_result result;

  registrations[0] = base_registration("accepted-helper");
  registrations[0].runtime_class = "imported-helper";
  registrations[0].compatible_component_roles = roles;
  registrations[0].compatible_component_role_count = 1u;

  registrations[1] = base_registration("narrowed-helper");
  registrations[1].runtime_class = "imported-helper";
  registrations[1].host_targets = mixed_hosts;
  registrations[1].host_target_count = 2u;
  registrations[1].compatible_component_roles = roles;
  registrations[1].compatible_component_role_count = 1u;

  registrations[2] = base_registration("future-runtime");
  registrations[2].runtime_class = "future-runtime-class";

  registrations[3] = base_registration("promoted-helper");
  registrations[3].runtime_class = "imported-helper";
  registrations[3].compatible_component_roles = roles;
  registrations[3].compatible_component_role_count = 1u;
  registrations[3].requested_persistent_class = "Symbiote";

  CHECK(cortex_catalog_ingest_batch(registrations, 4u, audits, 4u, &audit_count, &result) ==
        CORTEX_PERSISTENCE_OK);
  CHECK(audit_count == 4u);
  CHECK(audits[0].outcome == CORTEX_IMPORT_OUTCOME_ACCEPTED);
  CHECK(audits[1].outcome == CORTEX_IMPORT_OUTCOME_NARROWED);
  CHECK(audits[2].outcome == CORTEX_IMPORT_OUTCOME_QUARANTINED);
  CHECK(audits[3].outcome == CORTEX_IMPORT_OUTCOME_REJECTED);
  CHECK(audits[1].can_bind_as_subagent == 1u);
  CHECK(audits[3].import_error == CORTEX_ERR_IMPORT_PROMOTION_REQUIRED);
}

static void persists_and_reloads_audits(void) {
  static const char *roles[] = {"active"};
  cortex_catalog_registration registration = base_registration("accepted-helper-reload");
  cortex_persisted_import_audit audits[2];
  cortex_persisted_import_audit loaded_audits[2];
  cortex_runtime_state_snapshot snapshot;
  cortex_runtime_state_snapshot loaded;
  size_t audit_count = 0u;
  cortex_persistence_result result;

  registration.runtime_class = "imported-helper";
  registration.compatible_component_roles = roles;
  registration.compatible_component_role_count = 1u;
  snapshot = make_snapshot(audits, 2u, 0, 0u, 0, 0u, 0, 0u);
  loaded = make_snapshot(loaded_audits, 2u, 0, 0u, 0, 0u, 0, 0u);

  CHECK(cortex_catalog_ingest_batch(&registration, 1u, audits, 2u, &audit_count, &result) ==
        CORTEX_PERSISTENCE_OK);
  snapshot.audit_count = audit_count;
  CHECK(cortex_runtime_state_save("cortex_runtime_persistence_audit.state", &snapshot, &result) ==
        CORTEX_PERSISTENCE_OK);
  CHECK(cortex_runtime_state_load("cortex_runtime_persistence_audit.state", &loaded, &result) ==
        CORTEX_PERSISTENCE_OK);
  CHECK(loaded.audit_count == 1u);
  CHECK(strcmp(loaded_audits[0].registration_id, "accepted-helper-reload") == 0);
  CHECK(loaded_audits[0].outcome == CORTEX_IMPORT_OUTCOME_ACCEPTED);
  CHECK(loaded_audits[0].can_bind_as_subagent == 1u);
  (void)remove("cortex_runtime_persistence_audit.state");
}

static void captures_persists_and_reloads_character_runtime_state(void) {
  static const char *roles[] = {"active"};
  cortex_catalog_registration registration = base_registration("helper-roundtrip");
  cortex_import_mapping_result mapping;
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_subagent_instance subagent;
  cortex_runtime_result runtime_result;
  cortex_persisted_import_audit audits[2];
  cortex_persisted_import_audit loaded_audits[2];
  cortex_persisted_character_record characters[1];
  cortex_persisted_character_record loaded_characters[1];
  cortex_persisted_component_record persisted_components[2];
  cortex_persisted_component_record loaded_components[2];
  cortex_persisted_subagent_record subagents[1];
  cortex_persisted_subagent_record loaded_subagents[1];
  cortex_runtime_state_snapshot snapshot;
  cortex_runtime_state_snapshot loaded;
  size_t audit_count = 0u;
  cortex_persistence_result result;

  registration.runtime_class = "imported-helper";
  registration.compatible_component_roles = roles;
  registration.compatible_component_role_count = 1u;
  subagent.subagent_id = "subagent-roundtrip";
  subagent.definition_ref = 0;
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_DEFINED;
  subagent.imported_definition = 0u;

  CHECK(cortex_import_map_registration(&registration, &mapping) == CORTEX_IMPORT_OK);
  CHECK(cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &runtime_result) ==
        CORTEX_RUNTIME_OK);
  CHECK(cortex_subagent_apply_verb_for_component(&character, 0u, &subagent,
                                                 CORTEX_VERB_CONFIRM_SUBAGENT_SPAWN,
                                                 &runtime_result) == CORTEX_RUNTIME_OK);
  snapshot = make_snapshot(audits, 2u, characters, 1u, persisted_components, 2u, subagents, 1u);
  loaded = make_snapshot(loaded_audits, 2u, loaded_characters, 1u, loaded_components, 2u,
                         loaded_subagents, 1u);
  CHECK(cortex_catalog_ingest_batch(&registration, 1u, audits, 2u, &audit_count, &result) ==
        CORTEX_PERSISTENCE_OK);
  snapshot.audit_count = audit_count;
  CHECK(cortex_runtime_capture_character(&character, &subagent, 1u, &snapshot, &result) ==
        CORTEX_PERSISTENCE_OK);
  CHECK(cortex_runtime_state_save("cortex_runtime_persistence_character.state", &snapshot, &result) ==
        CORTEX_PERSISTENCE_OK);
  CHECK(cortex_runtime_state_load("cortex_runtime_persistence_character.state", &loaded, &result) ==
        CORTEX_PERSISTENCE_OK);

  CHECK(loaded.audit_count == 1u);
  CHECK(loaded.character_count == 1u);
  CHECK(loaded.component_count == 2u);
  CHECK(loaded.subagent_count == 1u);
  CHECK(strcmp(loaded_characters[0].character_id, "character-alpha") == 0);
  CHECK(loaded_characters[0].lifecycle_state == CORTEX_CHARACTER_ACTIVE);
  CHECK(loaded_components[0].live_or_queued_subagents == 1u);
  CHECK(loaded_subagents[0].lifecycle_state == CORTEX_SUBAGENT_ACTIVE);
  CHECK(strcmp(loaded_subagents[0].definition_ref, "helper-roundtrip") == 0);
  (void)remove("cortex_runtime_persistence_character.state");
}

static void rejects_schema_version_mismatch_without_count_mutation(void) {
  FILE *file = fopen("cortex_runtime_persistence_bad_schema.state", "wb");
  cortex_persisted_import_audit audits[1];
  cortex_runtime_state_snapshot snapshot;
  cortex_persistence_result result;

  CHECK(file != 0);
  if (file != 0) {
    (void)fprintf(file, "CORTEX_RUNTIME_STATE\t999\nCOUNTS\t0\t0\t0\t0\nEND\n");
    (void)fclose(file);
  }

  memset(audits, 0, sizeof(audits));
  snapshot = make_snapshot(audits, 1u, 0, 0u, 0, 0u, 0, 0u);
  snapshot.audit_count = 1u;
  CHECK(cortex_runtime_state_load("cortex_runtime_persistence_bad_schema.state",
                                  &snapshot, &result) ==
        CORTEX_ERR_PERSISTENCE_SCHEMA_MISMATCH);
  CHECK(snapshot.audit_count == 1u);
  CHECK(strcmp(result.rule_id, "schema_version_mismatch") == 0);
  (void)remove("cortex_runtime_persistence_bad_schema.state");
}

static void rejects_capture_of_invalid_runtime_state_without_mutation(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_persisted_character_record characters[1];
  cortex_persisted_component_record persisted_components[2];
  cortex_runtime_state_snapshot snapshot;
  cortex_persistence_result result;

  snapshot = make_snapshot(0, 0u, characters, 1u, persisted_components, 2u, 0, 0u);
  components[1].slot_index = 0u;

  CHECK(cortex_runtime_capture_character(&character, 0, 0u, &snapshot, &result) ==
        CORTEX_ERR_PERSISTENCE_RUNTIME_REJECTED);
  CHECK(snapshot.character_count == 0u);
  CHECK(snapshot.component_count == 0u);
  CHECK(strcmp(result.rule_id, "component_slot_duplicate") == 0);
}

static void rejects_corrupt_snapshot_counts_before_capture_or_save(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_persisted_character_record characters[1];
  cortex_persisted_component_record persisted_components[2];
  cortex_runtime_state_snapshot snapshot;
  cortex_persistence_result result;

  snapshot = make_snapshot(0, 0u, characters, 1u, persisted_components, 2u, 0, 0u);
  snapshot.component_count = 3u;

  CHECK(cortex_runtime_capture_character(&character, 0, 0u, &snapshot, &result) ==
        CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT);
  CHECK(snapshot.character_count == 0u);
  CHECK(snapshot.component_count == 3u);
  CHECK(cortex_runtime_state_save("cortex_runtime_persistence_corrupt_counts.state",
                                  &snapshot, &result) ==
        CORTEX_ERR_PERSISTENCE_INVALID_ARGUMENT);
  (void)remove("cortex_runtime_persistence_corrupt_counts.state");
}

static void rejects_unsupported_enum_values_on_load(void) {
  FILE *file = fopen("cortex_runtime_persistence_bad_enum.state", "wb");
  cortex_persisted_component_record components[1];
  cortex_runtime_state_snapshot snapshot;
  cortex_persistence_result result;

  CHECK(file != 0);
  if (file != 0) {
    (void)fprintf(file,
                  "CORTEX_RUNTIME_STATE\t1\n"
                  "COUNTS\t0\t0\t1\t0\n"
                  "M\tcomponent-active\tsymbiote-primary\t0\t99\t4\t12\t0\n"
                  "END\n");
    (void)fclose(file);
  }

  memset(components, 0, sizeof(components));
  snapshot = make_snapshot(0, 0u, 0, 0u, components, 1u, 0, 0u);
  CHECK(cortex_runtime_state_load("cortex_runtime_persistence_bad_enum.state",
                                  &snapshot, &result) ==
        CORTEX_ERR_PERSISTENCE_VALUE_UNSUPPORTED);
  CHECK(snapshot.component_count == 0u);
  (void)remove("cortex_runtime_persistence_bad_enum.state");
}

int main(void) {
  CHECK(cortex_runtime_persistence_abi_version() == 0x00010000u);
  CHECK(cortex_runtime_persistence_schema_version() == 1u);
  ingests_batch_and_records_all_mapper_outcomes();
  persists_and_reloads_audits();
  captures_persists_and_reloads_character_runtime_state();
  rejects_schema_version_mismatch_without_count_mutation();
  rejects_capture_of_invalid_runtime_state_without_mutation();
  rejects_corrupt_snapshot_counts_before_capture_or_save();
  rejects_unsupported_enum_values_on_load();
  CHECK(strcmp(cortex_persistence_error_name(CORTEX_PERSISTENCE_OK), "CORTEX_PERSISTENCE_OK") == 0);

  if (failures != 0) {
    fprintf(stderr, "%d runtime persistence test failure(s)\n", failures);
    return 1;
  }

  puts("runtime persistence tests passed");
  return 0;
}
