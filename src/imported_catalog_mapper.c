#include "cortex/imported_catalog_mapper.h"

#include <string.h>

#define CORTEX_DESCRIPTOR_VERSION 1u

static int is_empty(const char *value) {
  return value == 0 || value[0] == '\0';
}

static int equals(const char *left, const char *right) {
  return left != 0 && right != 0 && strcmp(left, right) == 0;
}

static int is_supported_host_target(const char *target) {
  return equals(target, "codex") ||
         equals(target, "openclaw") ||
         equals(target, "symbiosis-native") ||
         equals(target, "acp-bridge") ||
         equals(target, "VECTOR") ||
         equals(target, "NEXUS") ||
         equals(target, "vector") ||
         equals(target, "nexus");
}

static int is_helper_class(const char *runtime_class) {
  return equals(runtime_class, "helper") ||
         equals(runtime_class, "imported-helper") ||
         equals(runtime_class, "subagent-helper") ||
         equals(runtime_class, "ImportedHelperSubagentDefinition");
}

static int is_template_class(const char *runtime_class) {
  return equals(runtime_class, "array-template") ||
         equals(runtime_class, "imported-array-template") ||
         equals(runtime_class, "ImportedArrayTemplate");
}

static int is_overlay_class(const char *runtime_class) {
  return equals(runtime_class, "character-overlay") ||
         equals(runtime_class, "imported-character-overlay") ||
         equals(runtime_class, "overlay") ||
         equals(runtime_class, "ImportedCharacterOverlay");
}

static int is_persistent_identity_class(const char *class_name) {
  return equals(class_name, "Symbiote") ||
         equals(class_name, "Curator") ||
         equals(class_name, "Character") ||
         equals(class_name, "symbiote") ||
         equals(class_name, "curator") ||
         equals(class_name, "character");
}

static int is_valid_array_policy(const char *policy) {
  return equals(policy, "two_component") ||
         equals(policy, "three_component") ||
         equals(policy, "larger_justified");
}

static int is_personality_or_stylization_overlay(const char *overlay_kind) {
  return equals(overlay_kind, "personality") ||
         equals(overlay_kind, "stylization") ||
         equals(overlay_kind, "prism") ||
         equals(overlay_kind, "PRISM");
}

static void clear_result(cortex_import_mapping_result *result) {
  if (result == 0) {
    return;
  }

  result->surface = CORTEX_IMPORT_SURFACE_NONE;
  result->outcome = CORTEX_IMPORT_OUTCOME_REJECTED;
  result->lifecycle_state = CORTEX_IMPORT_LIFECYCLE_STAGED;
  result->error = CORTEX_ERR_IMPORT_INVALID_ARGUMENT;
  result->descriptor_id = 0;
  result->source_registration_ref = 0;
  result->rule_id = "invalid_argument";
  result->message = "invalid mapper argument";
  result->descriptor_version = CORTEX_DESCRIPTOR_VERSION;
  result->definition_only = 0u;
  result->can_bind_as_subagent = 0u;
}

static cortex_import_error finish(
  cortex_import_mapping_result *result,
  cortex_import_surface surface,
  cortex_import_outcome outcome,
  cortex_import_error error,
  const cortex_catalog_registration *registration,
  const char *rule_id,
  const char *message) {
  if (result != 0) {
    result->surface = surface;
    result->outcome = outcome;
    result->lifecycle_state = CORTEX_IMPORT_LIFECYCLE_STAGED;
    result->error = error;
    result->descriptor_id = registration != 0 ? registration->registration_id : 0;
    result->source_registration_ref = registration != 0 ? registration->registration_id : 0;
    result->rule_id = rule_id;
    result->message = message;
    result->descriptor_version = CORTEX_DESCRIPTOR_VERSION;
    result->definition_only = surface == CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION ? 1u : 0u;
    result->can_bind_as_subagent =
      surface == CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION &&
      outcome == CORTEX_IMPORT_OUTCOME_ACCEPTED ? 1u : 0u;
  }

  return error;
}

static cortex_import_error reject(
  cortex_import_mapping_result *result,
  cortex_import_error error,
  const cortex_catalog_registration *registration,
  const char *rule_id,
  const char *message) {
  return finish(result, CORTEX_IMPORT_SURFACE_NONE, CORTEX_IMPORT_OUTCOME_REJECTED,
                error, registration, rule_id, message);
}

static int has_valid_provenance(const cortex_catalog_registration *registration) {
  return !is_empty(registration->registration_id) &&
         !is_empty(registration->source_repo) &&
         !is_empty(registration->source_path) &&
         !is_empty(registration->source_commit);
}

static cortex_import_error validate_host_targets(
  const cortex_catalog_registration *registration,
  cortex_import_mapping_result *result,
  int *narrowed) {
  size_t index = 0u;
  size_t supported_count = 0u;

  if (narrowed != 0) {
    *narrowed = 0;
  }

  if (registration->host_target_count == 0u || registration->host_targets == 0) {
    return reject(result, CORTEX_ERR_EXPORT_HOST_TARGET_UNSUPPORTED, registration,
                  "host_target_required",
                  "at least one supported host target is required");
  }

  for (index = 0u; index < registration->host_target_count; ++index) {
    if (is_supported_host_target(registration->host_targets[index])) {
      ++supported_count;
    }
  }

  if (supported_count == 0u) {
    return reject(result, CORTEX_ERR_EXPORT_HOST_TARGET_UNSUPPORTED, registration,
                  "host_target_unsupported",
                  "host target is unsupported by the CORTEX external boundary");
  }

  if (supported_count < registration->host_target_count && narrowed != 0) {
    *narrowed = 1;
  }

  return CORTEX_IMPORT_OK;
}

static int requests_authority_replacement(const cortex_catalog_registration *registration) {
  return (registration->flags & CORTEX_IMPORT_FLAG_REPLACES_CHARACTER_OWNER) != 0u ||
         (registration->flags & CORTEX_IMPORT_FLAG_REPLACES_LIFECYCLE_AUTHORITY) != 0u ||
         (registration->flags & CORTEX_IMPORT_FLAG_REPLACES_CANONICAL_LABEL) != 0u;
}

static cortex_import_error validate_no_promotion(
  const cortex_catalog_registration *registration,
  cortex_import_mapping_result *result) {
  if ((registration->flags & CORTEX_IMPORT_FLAG_REQUESTS_PERSISTENT_IDENTITY) != 0u ||
      is_persistent_identity_class(registration->runtime_class) ||
      is_persistent_identity_class(registration->requested_persistent_class)) {
    return reject(result, CORTEX_ERR_IMPORT_PROMOTION_REQUIRED, registration,
                  "persistent_identity_promotion_blocked",
                  "imported descriptors may not become persistent identities by default");
  }

  if ((registration->flags & CORTEX_IMPORT_FLAG_REQUESTS_RUNTIME_ACTIVE_HELPER) != 0u) {
    return reject(result, CORTEX_ERR_IMPORT_PROMOTION_REQUIRED, registration,
                  "runtime_active_helper_requires_binding",
                  "imported helpers stay definition-only until bound as SubagentInstance");
  }

  return CORTEX_IMPORT_OK;
}

static cortex_import_error validate_subagent_limit_from_registration(
  const cortex_catalog_registration *registration,
  cortex_import_mapping_result *result) {
  uint32_t limit = registration->component_subagent_limit;
  if (limit == 0u) {
    limit = CORTEX_DEFAULT_SUBAGENT_LIMIT;
  }

  if (registration->requested_subagent_count > limit) {
    return reject(result, CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED, registration,
                  "component_subagent_limit_exceeded",
                  "requested helper binding would exceed component subagent limit");
  }

  return CORTEX_IMPORT_OK;
}

uint32_t cortex_imported_catalog_mapper_abi_version(void) {
  return (CORTEX_IMPORTED_CATALOG_MAPPER_ABI_VERSION_MAJOR << 16) |
         (CORTEX_IMPORTED_CATALOG_MAPPER_ABI_VERSION_MINOR << 8) |
         CORTEX_IMPORTED_CATALOG_MAPPER_ABI_VERSION_PATCH;
}

uint32_t cortex_imported_catalog_descriptor_version(void) {
  return CORTEX_DESCRIPTOR_VERSION;
}

cortex_import_error cortex_import_map_registration(
  const cortex_catalog_registration *registration,
  cortex_import_mapping_result *result) {
  cortex_import_error error = CORTEX_IMPORT_OK;
  int narrowed = 0;

  clear_result(result);

  if (registration == 0 || result == 0) {
    return CORTEX_ERR_IMPORT_INVALID_ARGUMENT;
  }

  if (!has_valid_provenance(registration)) {
    return reject(result, CORTEX_ERR_IMPORT_PROVENANCE_INVALID, registration,
                  "provenance_required",
                  "registration id, source repo, source path, and source commit are required");
  }

  error = validate_host_targets(registration, result, &narrowed);
  if (error != CORTEX_IMPORT_OK) {
    return error;
  }

  error = validate_no_promotion(registration, result);
  if (error != CORTEX_IMPORT_OK) {
    return error;
  }

  if (requests_authority_replacement(registration)) {
    return reject(result, CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION, registration,
                  "authority_replacement_blocked",
                  "imported descriptors may not replace owner, lifecycle, or label authority");
  }

  if (is_helper_class(registration->runtime_class)) {
    if (registration->compatible_component_role_count == 0u ||
        registration->compatible_component_roles == 0) {
      return reject(result, CORTEX_ERR_IMPORT_COMPONENT_INCOMPATIBLE, registration,
                    "helper_component_role_required",
                    "helper definitions require at least one compatible component role");
    }

    error = validate_subagent_limit_from_registration(registration, result);
    if (error != CORTEX_IMPORT_OK) {
      return error;
    }

    return finish(result, CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION,
                  narrowed ? CORTEX_IMPORT_OUTCOME_NARROWED : CORTEX_IMPORT_OUTCOME_ACCEPTED,
                  CORTEX_IMPORT_OK, registration,
                  narrowed ? "host_targets_narrowed" : "helper_definition_accepted",
                  narrowed ? "imported helper accepted after unsupported host targets were narrowed" :
                             "imported helper accepted as definition-only descriptor");
  }

  if (is_template_class(registration->runtime_class)) {
    if (!is_valid_array_policy(registration->array_binding_policy)) {
      return reject(result, CORTEX_ERR_IMPORT_ARRAY_POLICY_INVALID, registration,
                    "array_policy_invalid",
                    "array templates require a valid array binding policy");
    }

    return finish(result, CORTEX_IMPORT_SURFACE_ARRAY_TEMPLATE,
                  narrowed ? CORTEX_IMPORT_OUTCOME_NARROWED : CORTEX_IMPORT_OUTCOME_ACCEPTED,
                  CORTEX_IMPORT_OK, registration,
                  narrowed ? "host_targets_narrowed" : "array_template_accepted",
                  narrowed ? "imported array template accepted after unsupported host targets were narrowed" :
                             "imported array template accepted as construction seed");
  }

  if (is_overlay_class(registration->runtime_class)) {
    if (registration->blocked_authority_claim_count == 0u ||
        registration->blocked_authority_claims == 0) {
      return reject(result, CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION, registration,
                    "overlay_blocked_claims_required",
                    "overlays must declare blocked authority claims");
    }

    if (is_personality_or_stylization_overlay(registration->overlay_kind) &&
        is_empty(registration->prism_compatibility_ref)) {
      return reject(result, CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION, registration,
                    "overlay_prism_compatibility_required",
                    "personality or stylization overlays require PRISM compatibility");
    }

    return finish(result, CORTEX_IMPORT_SURFACE_CHARACTER_OVERLAY,
                  narrowed ? CORTEX_IMPORT_OUTCOME_NARROWED : CORTEX_IMPORT_OUTCOME_ACCEPTED,
                  CORTEX_IMPORT_OK, registration,
                  narrowed ? "host_targets_narrowed" : "character_overlay_accepted",
                  narrowed ? "imported character overlay accepted after unsupported host targets were narrowed" :
                             "imported character overlay accepted as bounded attachment");
  }

  return finish(result, CORTEX_IMPORT_SURFACE_QUARANTINE,
                CORTEX_IMPORT_OUTCOME_QUARANTINED,
                CORTEX_ERR_IMPORT_CLASS_UNSUPPORTED,
                registration,
                "runtime_class_quarantined",
                "runtime class has valid provenance but is not supported yet");
}

cortex_import_error cortex_import_validate_subagent_bind(
  uint32_t live_or_queued_subagents,
  uint32_t component_subagent_limit,
  cortex_import_mapping_result *result) {
  uint32_t limit = component_subagent_limit;
  clear_result(result);

  if (result == 0) {
    return CORTEX_ERR_IMPORT_INVALID_ARGUMENT;
  }

  if (limit == 0u) {
    limit = CORTEX_DEFAULT_SUBAGENT_LIMIT;
  }

  result->surface = CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION;
  result->source_registration_ref = 0;
  result->descriptor_id = 0;
  result->descriptor_version = CORTEX_DESCRIPTOR_VERSION;

  if (live_or_queued_subagents >= limit) {
    result->outcome = CORTEX_IMPORT_OUTCOME_REJECTED;
    result->lifecycle_state = CORTEX_IMPORT_LIFECYCLE_STAGED;
    result->error = CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED;
    result->rule_id = "component_subagent_limit_exceeded";
    result->message = "component already has the maximum live or queued subagents";
    result->definition_only = 1u;
    result->can_bind_as_subagent = 0u;
    return CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED;
  }

  result->outcome = CORTEX_IMPORT_OUTCOME_ACCEPTED;
  result->lifecycle_state = CORTEX_IMPORT_LIFECYCLE_STAGED;
  result->error = CORTEX_IMPORT_OK;
  result->rule_id = "subagent_bind_allowed";
  result->message = "component has capacity for one governed SubagentInstance";
  result->definition_only = 1u;
  result->can_bind_as_subagent = 1u;
  return CORTEX_IMPORT_OK;
}

const char *cortex_import_surface_name(cortex_import_surface surface) {
  switch (surface) {
    case CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION:
      return "ImportedHelperSubagentDefinition";
    case CORTEX_IMPORT_SURFACE_ARRAY_TEMPLATE:
      return "ImportedArrayTemplate";
    case CORTEX_IMPORT_SURFACE_CHARACTER_OVERLAY:
      return "ImportedCharacterOverlay";
    case CORTEX_IMPORT_SURFACE_QUARANTINE:
      return "Quarantine";
    case CORTEX_IMPORT_SURFACE_NONE:
    default:
      return "None";
  }
}

const char *cortex_import_outcome_name(cortex_import_outcome outcome) {
  switch (outcome) {
    case CORTEX_IMPORT_OUTCOME_ACCEPTED:
      return "accepted";
    case CORTEX_IMPORT_OUTCOME_NARROWED:
      return "narrowed";
    case CORTEX_IMPORT_OUTCOME_QUARANTINED:
      return "quarantined";
    case CORTEX_IMPORT_OUTCOME_REJECTED:
      return "rejected";
    default:
      return "unknown";
  }
}

const char *cortex_import_error_name(cortex_import_error error) {
  switch (error) {
    case CORTEX_IMPORT_OK:
      return "CORTEX_IMPORT_OK";
    case CORTEX_ERR_IMPORT_INVALID_ARGUMENT:
      return "CORTEX_ERR_IMPORT_INVALID_ARGUMENT";
    case CORTEX_ERR_IMPORT_PROVENANCE_INVALID:
      return "CORTEX_ERR_IMPORT_PROVENANCE_INVALID";
    case CORTEX_ERR_IMPORT_CLASS_UNSUPPORTED:
      return "CORTEX_ERR_IMPORT_CLASS_UNSUPPORTED";
    case CORTEX_ERR_IMPORT_SURFACE_AMBIGUOUS:
      return "CORTEX_ERR_IMPORT_SURFACE_AMBIGUOUS";
    case CORTEX_ERR_IMPORT_REFERENCE_UNRESOLVED:
      return "CORTEX_ERR_IMPORT_REFERENCE_UNRESOLVED";
    case CORTEX_ERR_IMPORT_ARRAY_POLICY_INVALID:
      return "CORTEX_ERR_IMPORT_ARRAY_POLICY_INVALID";
    case CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION:
      return "CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION";
    case CORTEX_ERR_IMPORT_COMPONENT_INCOMPATIBLE:
      return "CORTEX_ERR_IMPORT_COMPONENT_INCOMPATIBLE";
    case CORTEX_ERR_IMPORT_PROMOTION_REQUIRED:
      return "CORTEX_ERR_IMPORT_PROMOTION_REQUIRED";
    case CORTEX_ERR_EXPORT_HOST_TARGET_UNSUPPORTED:
      return "CORTEX_ERR_EXPORT_HOST_TARGET_UNSUPPORTED";
    case CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED:
      return "CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED";
    default:
      return "CORTEX_ERR_UNKNOWN";
  }
}
