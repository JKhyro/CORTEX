#ifndef CORTEX_IMPORTED_CATALOG_MAPPER_H
#define CORTEX_IMPORTED_CATALOG_MAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CORTEX_IMPORTED_CATALOG_MAPPER_ABI_VERSION_MAJOR 1u
#define CORTEX_IMPORTED_CATALOG_MAPPER_ABI_VERSION_MINOR 0u
#define CORTEX_IMPORTED_CATALOG_MAPPER_ABI_VERSION_PATCH 0u

#define CORTEX_DEFAULT_SUBAGENT_LIMIT 12u

typedef enum cortex_import_surface {
  CORTEX_IMPORT_SURFACE_NONE = 0,
  CORTEX_IMPORT_SURFACE_HELPER_SUBAGENT_DEFINITION = 1,
  CORTEX_IMPORT_SURFACE_ARRAY_TEMPLATE = 2,
  CORTEX_IMPORT_SURFACE_CHARACTER_OVERLAY = 3,
  CORTEX_IMPORT_SURFACE_QUARANTINE = 4
} cortex_import_surface;

typedef enum cortex_import_outcome {
  CORTEX_IMPORT_OUTCOME_ACCEPTED = 0,
  CORTEX_IMPORT_OUTCOME_NARROWED = 1,
  CORTEX_IMPORT_OUTCOME_QUARANTINED = 2,
  CORTEX_IMPORT_OUTCOME_REJECTED = 3
} cortex_import_outcome;

typedef enum cortex_import_lifecycle_state {
  CORTEX_IMPORT_LIFECYCLE_STAGED = 0,
  CORTEX_IMPORT_LIFECYCLE_VALIDATED = 1,
  CORTEX_IMPORT_LIFECYCLE_BOUND = 2,
  CORTEX_IMPORT_LIFECYCLE_RETIRED = 3
} cortex_import_lifecycle_state;

typedef enum cortex_import_error {
  CORTEX_IMPORT_OK = 0,
  CORTEX_ERR_IMPORT_INVALID_ARGUMENT = 1,
  CORTEX_ERR_IMPORT_PROVENANCE_INVALID = 2,
  CORTEX_ERR_IMPORT_CLASS_UNSUPPORTED = 3,
  CORTEX_ERR_IMPORT_SURFACE_AMBIGUOUS = 4,
  CORTEX_ERR_IMPORT_REFERENCE_UNRESOLVED = 5,
  CORTEX_ERR_IMPORT_ARRAY_POLICY_INVALID = 6,
  CORTEX_ERR_IMPORT_OVERLAY_AUTHORITY_VIOLATION = 7,
  CORTEX_ERR_IMPORT_COMPONENT_INCOMPATIBLE = 8,
  CORTEX_ERR_IMPORT_PROMOTION_REQUIRED = 9,
  CORTEX_ERR_EXPORT_HOST_TARGET_UNSUPPORTED = 10,
  CORTEX_ERR_SUBAGENT_LIMIT_EXCEEDED = 11
} cortex_import_error;

typedef enum cortex_import_registration_flags {
  CORTEX_IMPORT_FLAG_NONE = 0u,
  CORTEX_IMPORT_FLAG_REQUESTS_PERSISTENT_IDENTITY = 1u << 0,
  CORTEX_IMPORT_FLAG_REPLACES_CHARACTER_OWNER = 1u << 1,
  CORTEX_IMPORT_FLAG_REPLACES_LIFECYCLE_AUTHORITY = 1u << 2,
  CORTEX_IMPORT_FLAG_REPLACES_CANONICAL_LABEL = 1u << 3,
  CORTEX_IMPORT_FLAG_REQUESTS_RUNTIME_ACTIVE_HELPER = 1u << 4
} cortex_import_registration_flags;

typedef struct cortex_catalog_registration {
  const char *registration_id;
  const char *source_repo;
  const char *source_path;
  const char *source_commit;
  const char *source_manifest_ref;
  const char *runtime_class;
  const char *canonical_label;
  const char *array_binding_policy;
  const char *overlay_kind;
  const char *prism_compatibility_ref;
  const char *requested_persistent_class;
  const char *promotion_policy;
  const char *compatibility_version;
  const char *const *host_targets;
  size_t host_target_count;
  const char *const *compatible_component_roles;
  size_t compatible_component_role_count;
  const char *const *blocked_authority_claims;
  size_t blocked_authority_claim_count;
  uint32_t requested_subagent_count;
  uint32_t component_subagent_limit;
  uint32_t flags;
} cortex_catalog_registration;

typedef struct cortex_import_mapping_result {
  cortex_import_surface surface;
  cortex_import_outcome outcome;
  cortex_import_lifecycle_state lifecycle_state;
  cortex_import_error error;
  const char *descriptor_id;
  const char *source_registration_ref;
  const char *rule_id;
  const char *message;
  uint32_t descriptor_version;
  uint8_t definition_only;
  uint8_t can_bind_as_subagent;
} cortex_import_mapping_result;

uint32_t cortex_imported_catalog_mapper_abi_version(void);
uint32_t cortex_imported_catalog_descriptor_version(void);

cortex_import_error cortex_import_map_registration(
  const cortex_catalog_registration *registration,
  cortex_import_mapping_result *result);

cortex_import_error cortex_import_validate_subagent_bind(
  uint32_t live_or_queued_subagents,
  uint32_t component_subagent_limit,
  cortex_import_mapping_result *result);

const char *cortex_import_surface_name(cortex_import_surface surface);
const char *cortex_import_outcome_name(cortex_import_outcome outcome);
const char *cortex_import_error_name(cortex_import_error error);

#ifdef __cplusplus
}
#endif

#endif
