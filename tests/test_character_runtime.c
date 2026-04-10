#include "cortex/character_runtime.h"

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

static cortex_character make_two_component_character(cortex_ara_component components[2]) {
  cortex_character character;

  components[0].component_id = "component-active";
  components[0].slot_index = 0u;
  components[0].role = CORTEX_COMPONENT_ROLE_ACTIVE;
  components[0].owner_identity_ref = "symbiote-primary";
  components[0].lifecycle_state = CORTEX_COMPONENT_READY;
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
  character.lifecycle_state = CORTEX_CHARACTER_DRAFT;
  character.components = components;
  character.component_count = 2u;
  return character;
}

static cortex_import_mapping_result accepted_helper_mapping(void) {
  cortex_import_mapping_result mapping;

  mapping.surface = CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION;
  mapping.outcome = CORTEX_IMPORT_OUTCOME_ACCEPTED;
  mapping.lifecycle_state = CORTEX_IMPORT_LIFECYCLE_STAGED;
  mapping.error = CORTEX_IMPORT_OK;
  mapping.descriptor_id = "helper-empathy-v2";
  mapping.source_registration_ref = "catalog-entry-1";
  mapping.rule_id = "helper_definition_accepted";
  mapping.message = "accepted helper";
  mapping.descriptor_version = cortex_imported_catalog_descriptor_version();
  mapping.definition_only = 1u;
  mapping.can_bind_as_subagent = 1u;
  return mapping;
}

static void validates_and_activates_two_component_character(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_runtime_result result;

  CHECK(cortex_character_runtime_abi_version() == 0x00010000u);
  CHECK(cortex_character_runtime_descriptor_version() == 1u);
  CHECK(cortex_character_validate(&character, &result) == CORTEX_RUNTIME_OK);
  CHECK(strcmp(result.rule_id, "character_validated") == 0);
  CHECK(cortex_character_apply_verb(&character, CORTEX_VERB_VALIDATE_CHARACTER, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(character.lifecycle_state == CORTEX_CHARACTER_VALIDATED);
  CHECK(cortex_character_apply_verb(&character, CORTEX_VERB_MARK_READY, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(character.lifecycle_state == CORTEX_CHARACTER_READY);
  CHECK(cortex_character_apply_verb(&character, CORTEX_VERB_ACTIVATE_CHARACTER, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(character.lifecycle_state == CORTEX_CHARACTER_ACTIVE);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_ACTIVE);
  CHECK(components[1].lifecycle_state == CORTEX_COMPONENT_ACTIVE);
}

static void validates_three_component_character(void) {
  cortex_ara_component components[3];
  cortex_character character;
  cortex_runtime_result result;

  (void)make_two_component_character(components);
  components[2].component_id = "component-reasoning";
  components[2].slot_index = 2u;
  components[2].role = CORTEX_COMPONENT_ROLE_REASONING;
  components[2].owner_identity_ref = "symbiote-primary";
  components[2].lifecycle_state = CORTEX_COMPONENT_READY;
  components[2].subagent_limit = CORTEX_DEFAULT_SUBAGENT_LIMIT;
  components[2].live_or_queued_subagents = 0u;

  character.character_id = "character-beta";
  character.owner_identity_ref = "symbiote-primary";
  character.array_shape = CORTEX_ARA_SHAPE_THREE_COMPONENT;
  character.lifecycle_state = CORTEX_CHARACTER_DRAFT;
  character.components = components;
  character.component_count = 3u;

  CHECK(cortex_character_validate(&character, &result) == CORTEX_RUNTIME_OK);
}

static void rejects_duplicate_slot_without_mutation(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_runtime_result result;

  components[1].slot_index = 0u;

  CHECK(cortex_character_validate(&character, &result) == CORTEX_ERR_RUNTIME_DUPLICATE_SLOT);
  CHECK(character.lifecycle_state == CORTEX_CHARACTER_DRAFT);
  CHECK(result.failing_index == 1u);
}

static void rejects_missing_required_role_set(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_runtime_result result;

  components[1].role = CORTEX_COMPONENT_ROLE_ACTIVE;

  CHECK(cortex_character_validate(&character, &result) ==
        CORTEX_ERR_RUNTIME_INVALID_COMPONENT_ROLE);
  CHECK(strcmp(result.rule_id, "array_required_roles_missing") == 0);
  CHECK(character.lifecycle_state == CORTEX_CHARACTER_DRAFT);
}

static void rejects_invalid_transition_without_mutation(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_runtime_result result;

  CHECK(cortex_character_apply_verb(&character, CORTEX_VERB_ACTIVATE_CHARACTER, &result) ==
        CORTEX_ERR_RUNTIME_INVALID_TRANSITION);
  CHECK(character.lifecycle_state == CORTEX_CHARACTER_DRAFT);
}

static void rejects_component_activation_when_parent_not_ready(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_runtime_result result;

  CHECK(cortex_component_apply_verb(&character, 0u, CORTEX_VERB_ACTIVATE_COMPONENT, &result) ==
        CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_READY);
}

static void transitions_component_suspend_degrade_detach_archive(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_runtime_result result;

  character.lifecycle_state = CORTEX_CHARACTER_ACTIVE;
  components[0].lifecycle_state = CORTEX_COMPONENT_ACTIVE;

  CHECK(cortex_component_apply_verb(&character, 0u, CORTEX_VERB_SUSPEND_COMPONENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_SUSPENDED);
  CHECK(cortex_component_apply_verb(&character, 0u, CORTEX_VERB_RESUME_COMPONENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_ACTIVE);
  CHECK(cortex_component_apply_verb(&character, 0u, CORTEX_VERB_DEGRADE_COMPONENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_DEGRADED);
  CHECK(cortex_component_apply_verb(&character, 0u, CORTEX_VERB_RESTORE_COMPONENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_ACTIVE);
  CHECK(cortex_component_apply_verb(&character, 0u, CORTEX_VERB_DETACH_COMPONENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_DETACHED);
  CHECK(cortex_component_apply_verb(&character, 0u, CORTEX_VERB_ARCHIVE_COMPONENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_ARCHIVED);
}

static void blocks_component_archive_with_live_children(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_runtime_result result;

  components[0].lifecycle_state = CORTEX_COMPONENT_ACTIVE;
  components[0].live_or_queued_subagents = 1u;

  CHECK(cortex_component_apply_verb(&character, 0u, CORTEX_VERB_ARCHIVE_COMPONENT, &result) ==
        CORTEX_ERR_RUNTIME_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN);
  CHECK(components[0].lifecycle_state == CORTEX_COMPONENT_ACTIVE);
  CHECK(components[0].live_or_queued_subagents == 1u);
}

static void binds_imported_helper_through_component_only(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_import_mapping_result mapping = accepted_helper_mapping();
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  subagent.subagent_id = "subagent-1";
  subagent.definition_ref = 0;
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_DEFINED;
  subagent.imported_definition = 0u;

  CHECK(cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_QUEUED);
  CHECK(subagent.imported_definition == 1u);
  CHECK(components[0].live_or_queued_subagents == 1u);
  CHECK(strcmp(subagent.definition_ref, "helper-empathy-v2") == 0);
}

static void binds_narrowed_imported_helper_through_component(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_import_mapping_result mapping = accepted_helper_mapping();
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  mapping.outcome = CORTEX_IMPORT_OUTCOME_NARROWED;
  mapping.rule_id = "host_targets_narrowed";
  mapping.can_bind_as_subagent = 1u;
  subagent.subagent_id = "subagent-narrowed";
  subagent.definition_ref = 0;
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_DEFINED;
  subagent.imported_definition = 0u;

  CHECK(cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_QUEUED);
  CHECK(subagent.imported_definition == 1u);
  CHECK(components[0].live_or_queued_subagents == 1u);
}

static void binds_mapper_produced_narrowed_helper_through_component(void) {
  static const char *host_targets[] = {"codex", "root-shell"};
  static const char *roles[] = {"active"};
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_catalog_registration registration;
  cortex_import_mapping_result mapping;
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  memset(&registration, 0, sizeof(registration));
  registration.registration_id = "catalog-entry-narrowed";
  registration.source_repo = "JKhyro/SYMBIOSIS";
  registration.source_path = ".codex/agents/helper.toml";
  registration.source_commit = "5f855c11f9117541da31e7274b738cc396d4d3c7";
  registration.runtime_class = "imported-helper";
  registration.canonical_label = "Imported helper";
  registration.compatibility_version = "1";
  registration.host_targets = host_targets;
  registration.host_target_count = 2u;
  registration.compatible_component_roles = roles;
  registration.compatible_component_role_count = 1u;

  subagent.subagent_id = "subagent-mapper-produced-narrowed";
  subagent.definition_ref = 0;
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_DEFINED;
  subagent.imported_definition = 0u;

  CHECK(cortex_import_map_registration(&registration, &mapping) == CORTEX_IMPORT_OK);
  CHECK(mapping.outcome == CORTEX_IMPORT_OUTCOME_NARROWED);
  CHECK(mapping.can_bind_as_subagent == 1u);
  CHECK(cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_QUEUED);
  CHECK(components[0].live_or_queued_subagents == 1u);
}

static void rejects_rejected_import_mapping_without_mutation(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_import_mapping_result mapping = accepted_helper_mapping();
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  mapping.outcome = CORTEX_IMPORT_OUTCOME_REJECTED;
  mapping.error = CORTEX_ERR_IMPORT_PROMOTION_REQUIRED;
  mapping.can_bind_as_subagent = 0u;
  subagent.subagent_id = "subagent-2";
  subagent.definition_ref = 0;
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_DEFINED;
  subagent.imported_definition = 0u;

  CHECK(cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &result) ==
        CORTEX_ERR_RUNTIME_IMPORT_MAPPING_REJECTED);
  CHECK(components[0].live_or_queued_subagents == 0u);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_DEFINED);
}

static void rejects_bind_from_non_defined_subagent_without_mutation(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_import_mapping_result mapping = accepted_helper_mapping();
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  components[0].lifecycle_state = CORTEX_COMPONENT_ACTIVE;
  subagent.subagent_id = "subagent-live";
  subagent.definition_ref = "helper-existing";
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_ACTIVE;
  subagent.imported_definition = 1u;

  CHECK(cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &result) ==
        CORTEX_ERR_RUNTIME_INVALID_TRANSITION);
  CHECK(components[0].live_or_queued_subagents == 0u);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_ACTIVE);
  CHECK(strcmp(subagent.definition_ref, "helper-existing") == 0);
}

static void rejects_subagent_over_limit_without_mutation(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_import_mapping_result mapping = accepted_helper_mapping();
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  components[0].live_or_queued_subagents = CORTEX_DEFAULT_SUBAGENT_LIMIT;
  subagent.subagent_id = "subagent-3";
  subagent.lifecycle_state = CORTEX_SUBAGENT_DEFINED;
  subagent.imported_definition = 0u;

  CHECK(cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &result) ==
        CORTEX_ERR_RUNTIME_SUBAGENT_LIMIT_EXCEEDED);
  CHECK(components[0].live_or_queued_subagents == CORTEX_DEFAULT_SUBAGENT_LIMIT);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_DEFINED);
}

static void blocks_archive_with_live_children(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_runtime_result result;

  character.lifecycle_state = CORTEX_CHARACTER_ACTIVE;
  components[0].live_or_queued_subagents = 1u;

  CHECK(cortex_character_apply_verb(&character, CORTEX_VERB_ARCHIVE_CHARACTER, &result) ==
        CORTEX_ERR_RUNTIME_ARCHIVE_BLOCKED_BY_LIVE_CHILDREN);
  CHECK(character.lifecycle_state == CORTEX_CHARACTER_ACTIVE);
}

static void transitions_subagent_spawn_and_archive(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  character.lifecycle_state = CORTEX_CHARACTER_ACTIVE;
  components[0].lifecycle_state = CORTEX_COMPONENT_ACTIVE;
  components[0].live_or_queued_subagents = 1u;
  subagent.subagent_id = "subagent-4";
  subagent.definition_ref = "helper-empathy-v2";
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_QUEUED;
  subagent.imported_definition = 1u;

  CHECK(cortex_subagent_apply_verb(&subagent, CORTEX_VERB_CONFIRM_SUBAGENT_SPAWN, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_ACTIVE);
  CHECK(cortex_subagent_apply_verb(&subagent, CORTEX_VERB_SUSPEND_SUBAGENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_SUSPENDED);
  CHECK(cortex_subagent_apply_verb(&subagent, CORTEX_VERB_RESUME_SUBAGENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_ACTIVE);
  CHECK(cortex_subagent_apply_verb_for_component(&character, 0u, &subagent,
                                                 CORTEX_VERB_DESPAWN_SUBAGENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_DESPAWNED);
  CHECK(components[0].live_or_queued_subagents == 0u);
  CHECK(cortex_subagent_apply_verb(&subagent, CORTEX_VERB_ARCHIVE_SUBAGENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_ARCHIVED);
}

static void context_free_despawn_requires_component_context(void) {
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  subagent.subagent_id = "subagent-context-required";
  subagent.definition_ref = "helper-empathy-v2";
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_ACTIVE;
  subagent.imported_definition = 1u;

  CHECK(cortex_subagent_apply_verb(&subagent, CORTEX_VERB_DESPAWN_SUBAGENT, &result) ==
        CORTEX_ERR_RUNTIME_PARENT_STATE_CONFLICT);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_ACTIVE);
}

static void subagent_despawn_releases_component_capacity_and_allows_archive(void) {
  cortex_ara_component components[2];
  cortex_character character = make_two_component_character(components);
  cortex_import_mapping_result mapping = accepted_helper_mapping();
  cortex_subagent_instance subagent;
  cortex_runtime_result result;

  character.lifecycle_state = CORTEX_CHARACTER_ACTIVE;
  components[0].lifecycle_state = CORTEX_COMPONENT_ACTIVE;
  subagent.subagent_id = "subagent-5";
  subagent.definition_ref = 0;
  subagent.component_index = 0u;
  subagent.lifecycle_state = CORTEX_SUBAGENT_DEFINED;
  subagent.imported_definition = 0u;

  CHECK(cortex_subagent_bind_imported_helper(&character, 0u, &mapping, &subagent, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(components[0].live_or_queued_subagents == 1u);
  CHECK(cortex_subagent_apply_verb_for_component(&character, 0u, &subagent,
                                                 CORTEX_VERB_CONFIRM_SUBAGENT_SPAWN, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_ACTIVE);
  CHECK(cortex_subagent_apply_verb_for_component(&character, 0u, &subagent,
                                                 CORTEX_VERB_DESPAWN_SUBAGENT, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(subagent.lifecycle_state == CORTEX_SUBAGENT_DESPAWNED);
  CHECK(components[0].live_or_queued_subagents == 0u);
  CHECK(cortex_character_apply_verb(&character, CORTEX_VERB_ARCHIVE_CHARACTER, &result) ==
        CORTEX_RUNTIME_OK);
  CHECK(character.lifecycle_state == CORTEX_CHARACTER_ARCHIVED);
}

int main(void) {
  validates_and_activates_two_component_character();
  validates_three_component_character();
  rejects_duplicate_slot_without_mutation();
  rejects_missing_required_role_set();
  rejects_invalid_transition_without_mutation();
  rejects_component_activation_when_parent_not_ready();
  transitions_component_suspend_degrade_detach_archive();
  blocks_component_archive_with_live_children();
  binds_imported_helper_through_component_only();
  binds_narrowed_imported_helper_through_component();
  binds_mapper_produced_narrowed_helper_through_component();
  rejects_rejected_import_mapping_without_mutation();
  rejects_bind_from_non_defined_subagent_without_mutation();
  rejects_subagent_over_limit_without_mutation();
  blocks_archive_with_live_children();
  transitions_subagent_spawn_and_archive();
  context_free_despawn_requires_component_context();
  subagent_despawn_releases_component_capacity_and_allows_archive();

  if (failures != 0) {
    fprintf(stderr, "%d character runtime test failure(s)\n", failures);
    return 1;
  }

  puts("character runtime tests passed");
  return 0;
}
