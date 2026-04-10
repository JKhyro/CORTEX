#include "cortex/imported_catalog_mapper.h"

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

static cortex_catalog_registration base_registration(void) {
  static const char *host_targets[] = {"codex", "symbiosis-native"};
  cortex_catalog_registration registration;

  memset(&registration, 0, sizeof(registration));
  registration.registration_id = "catalog-entry-1";
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

static void accepts_helper_definition_only(void) {
  static const char *roles[] = {"active", "memory"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "imported-helper";
  registration.compatible_component_roles = roles;
  registration.compatible_component_role_count = 2u;
  registration.requested_subagent_count = 1u;

  CHECK(cortex_import_map_registration(&registration, &result) == CORTEX_IMPORT_OK);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_ACCEPTED);
  CHECK(result.surface == CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION);
  CHECK(result.lifecycle_state == CORTEX_IMPORT_LIFECYCLE_STAGED);
  CHECK(result.definition_only == 1u);
  CHECK(result.can_bind_as_subagent == 1u);
  CHECK(strcmp(cortex_import_surface_name(result.surface), "ImportedHelperSubagentDefinition") == 0);
}

static void accepts_array_template(void) {
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "array-template";
  registration.array_binding_policy = "three_component";

  CHECK(cortex_import_map_registration(&registration, &result) == CORTEX_IMPORT_OK);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_ACCEPTED);
  CHECK(result.surface == CORTEX_IMPORT_SURFACE_ARRAY_TEMPLATE);
  CHECK(result.definition_only == 0u);
  CHECK(result.error == CORTEX_IMPORT_OK);
}

static void accepts_character_overlay(void) {
  static const char *blocked_claims[] = {"owner", "lifecycle", "canonical_label"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "character-overlay";
  registration.overlay_kind = "governance";
  registration.blocked_authority_claims = blocked_claims;
  registration.blocked_authority_claim_count = 3u;

  CHECK(cortex_import_map_registration(&registration, &result) == CORTEX_IMPORT_OK);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_ACCEPTED);
  CHECK(result.surface == CORTEX_IMPORT_SURFACE_CHARACTER_OVERLAY);
  CHECK(result.error == CORTEX_IMPORT_OK);
}

static void narrows_mixed_host_targets(void) {
  static const char *host_targets[] = {"codex", "root-shell"};
  static const char *roles[] = {"active"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "imported-helper";
  registration.host_targets = host_targets;
  registration.host_target_count = 2u;
  registration.compatible_component_roles = roles;
  registration.compatible_component_role_count = 1u;

  CHECK(cortex_import_map_registration(&registration, &result) == CORTEX_IMPORT_OK);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_NARROWED);
  CHECK(result.surface == CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION);
  CHECK(result.error == CORTEX_IMPORT_OK);
  CHECK(result.can_bind_as_subagent == 1u);
  CHECK(strcmp(result.rule_id, "host_targets_narrowed") == 0);
  CHECK(strcmp(cortex_import_outcome_name(result.outcome), "narrowed") == 0);
}

static void narrows_array_template_mixed_host_targets(void) {
  static const char *host_targets[] = {"nexus", "root-shell"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "array-template";
  registration.host_targets = host_targets;
  registration.host_target_count = 2u;
  registration.array_binding_policy = "two_component";

  CHECK(cortex_import_map_registration(&registration, &result) == CORTEX_IMPORT_OK);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_NARROWED);
  CHECK(result.surface == CORTEX_IMPORT_SURFACE_ARRAY_TEMPLATE);
  CHECK(result.error == CORTEX_IMPORT_OK);
  CHECK(strcmp(result.rule_id, "host_targets_narrowed") == 0);
}

static void narrows_overlay_mixed_host_targets(void) {
  static const char *host_targets[] = {"VECTOR", "root-shell"};
  static const char *blocked_claims[] = {"owner", "lifecycle", "canonical_label"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "character-overlay";
  registration.host_targets = host_targets;
  registration.host_target_count = 2u;
  registration.overlay_kind = "governance";
  registration.blocked_authority_claims = blocked_claims;
  registration.blocked_authority_claim_count = 3u;

  CHECK(cortex_import_map_registration(&registration, &result) == CORTEX_IMPORT_OK);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_NARROWED);
  CHECK(result.surface == CORTEX_IMPORT_SURFACE_CHARACTER_OVERLAY);
  CHECK(result.error == CORTEX_IMPORT_OK);
  CHECK(strcmp(result.rule_id, "host_targets_narrowed") == 0);
}

static void quarantines_unknown_runtime_class(void) {
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "future-runtime-class";

  CHECK(cortex_import_map_registration(&registration, &result) == CORTEX_ERR_IMPORT_CLASS_UNSUPPORTED);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_QUARANTINED);
  CHECK(result.surface == CORTEX_IMPORT_SURFACE_QUARANTINE);
  CHECK(result.error == CORTEX_ERR_IMPORT_CLASS_UNSUPPORTED);
}

static void rejects_persistent_identity_promotion(void) {
  static const char *roles[] = {"active"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "imported-helper";
  registration.compatible_component_roles = roles;
  registration.compatible_component_role_count = 1u;
  registration.requested_persistent_class = "Symbiote";

  CHECK(cortex_import_map_registration(&registration, &result) == CORTEX_ERR_IMPORT_PROMOTION_REQUIRED);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_REJECTED);
  CHECK(result.error == CORTEX_ERR_IMPORT_PROMOTION_REQUIRED);
}

static void rejects_owner_or_lifecycle_replacement(void) {
  static const char *blocked_claims[] = {"owner", "lifecycle"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "character-overlay";
  registration.overlay_kind = "governance";
  registration.blocked_authority_claims = blocked_claims;
  registration.blocked_authority_claim_count = 2u;
  registration.flags = CORTEX_IMPORT_FLAG_REPLACES_CHARACTER_OWNER |
                       CORTEX_IMPORT_FLAG_REPLACES_LIFECYCLE_AUTHORITY;

  CHECK(cortex_import_map_registration(&registration, &result) ==
        CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_REJECTED);
  CHECK(result.error == CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION);
}

static void rejects_unsupported_host_target(void) {
  static const char *host_targets[] = {"root-shell"};
  static const char *roles[] = {"active"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "imported-helper";
  registration.host_targets = host_targets;
  registration.host_target_count = 1u;
  registration.compatible_component_roles = roles;
  registration.compatible_component_role_count = 1u;

  CHECK(cortex_import_map_registration(&registration, &result) ==
        CORTEX_ERR_EXPORT_HOST_TARGET_UNSUPPORTED);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_REJECTED);
  CHECK(result.error == CORTEX_ERR_EXPORT_HOST_TARGET_UNSUPPORTED);
}

static void rejects_invalid_overlay_prism_compatibility(void) {
  static const char *blocked_claims[] = {"owner", "lifecycle", "canonical_label"};
  cortex_import_mapping_result result;
  cortex_catalog_registration registration = base_registration();

  registration.runtime_class = "character-overlay";
  registration.overlay_kind = "stylization";
  registration.blocked_authority_claims = blocked_claims;
  registration.blocked_authority_claim_count = 3u;

  CHECK(cortex_import_map_registration(&registration, &result) ==
        CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_REJECTED);
  CHECK(result.error == CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION);
}

static void rejects_subagent_limit_exceeded(void) {
  cortex_import_mapping_result result;

  CHECK(cortex_import_validate_subagent_bind(CORTEX_DEFAULT_SUBAGENT_LIMIT,
                                             CORTEX_DEFAULT_SUBAGENT_LIMIT,
                                             &result) ==
        CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_REJECTED);
  CHECK(result.error == CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED);
  CHECK(result.can_bind_as_subagent == 0u);
}

static void accepts_subagent_bind_with_capacity(void) {
  cortex_import_mapping_result result;

  CHECK(cortex_import_validate_subagent_bind(3u, CORTEX_DEFAULT_SUBAGENT_LIMIT, &result) ==
        CORTEX_IMPORT_OK);
  CHECK(result.outcome == CORTEX_IMPORT_OUTCOME_ACCEPTED);
  CHECK(result.can_bind_as_subagent == 1u);
}

int main(void) {
  CHECK(cortex_imported_catalog_mapper_abi_version() == 0x00010000u);
  CHECK(cortex_imported_catalog_descriptor_version() == 1u);
  accepts_helper_definition_only();
  accepts_array_template();
  accepts_character_overlay();
  narrows_mixed_host_targets();
  narrows_array_template_mixed_host_targets();
  narrows_overlay_mixed_host_targets();
  quarantines_unknown_runtime_class();
  rejects_persistent_identity_promotion();
  rejects_owner_or_lifecycle_replacement();
  rejects_unsupported_host_target();
  rejects_invalid_overlay_prism_compatibility();
  rejects_subagent_limit_exceeded();
  accepts_subagent_bind_with_capacity();

  if (failures != 0) {
    fprintf(stderr, "%d imported catalog mapper test failure(s)\n", failures);
    return 1;
  }

  puts("imported catalog mapper tests passed");
  return 0;
}
