# CORTEX Packaging And Distribution Model

Issue alignment: `CORTEX #8`

## Purpose

Define how `CORTEX` is packaged and distributed when the product uses a native C core and an optional Avalonia desktop shell.

Packaging must preserve the native core as a first-class release output. The desktop shell may be shipped as a companion host, but it must not replace or hide the canonical native runtime.

This contract assumes the runtime topology in [`runtime-topology.md`](./runtime-topology.md), the stack rule in [`native-c-avalonia-direction.md`](./native-c-avalonia-direction.md), and the lifecycle and entity rules already defined by CORTEX.

## Interface contract

The packaging model must expose four release surfaces:

- `cortex-core-native`: the versioned native core artifact, including the canonical C ABI and any required native dependencies
- `cortex-cli-or-native-host`: a native executable or host entry point that can validate and exercise the core without the desktop shell
- `cortex-desktop-bundle`: an optional Avalonia desktop package that ships with or locates the matching native core explicitly
- `cortex-dev-sdk`: headers, ABI notes, and packaging metadata needed to verify native compatibility and host integration

Each release surface must declare the exact core version it is compatible with. A shell or helper package must never imply compatibility with an unknown native core build.

## Expected inputs and outputs

Inputs into the packaging process:

- the compiled native core library and ABI descriptor
- a native host executable or smoke-test harness
- optional Avalonia desktop host binaries and packaging glue
- platform target metadata, starting with Windows-first distribution
- release version, build metadata, and compatibility matrix information

Outputs from the packaging process:

- native-only release artifacts that can be run or validated without the Avalonia shell
- desktop bundles that either include the exact matching native core or fail fast when it is missing
- release manifests that name artifact versions, platform targets, and compatibility constraints
- verification evidence that native validation succeeds independently of the desktop host

## Artifact model

### Native-first artifacts

- The native core must be packaged as an explicit artifact, not only as a hidden file inside a desktop bundle.
- The native host or CLI validation path must also be packaged so the runtime can be tested without the desktop shell.
- Debug symbols, headers, and ABI descriptors should be published as developer-facing companion artifacts where appropriate.

### Desktop bundle

- The Avalonia desktop package may be distributed as a separate bundle or as a companion package to the native core.
- The desktop bundle must declare and verify the exact native core version it expects.
- If the bundle locates the native core externally, the discovery path and failure behavior must be explicit.

### Release classes

- `native-runtime`: native core plus native host, suitable for validation and headless operation
- `desktop-runtime`: native core plus Avalonia shell plus host-specific bootstrap glue
- `developer-sdk`: headers, ABI docs, packaging metadata, and validation helpers for integration work

## Distribution rules

- Windows is the first supported packaging target.
- Native artifacts must remain directly accessible for testing, diagnostics, and future non-desktop hosting.
- The desktop shell must not ship a private, undocumented copy of the core that cannot be independently verified.
- Packaging should prefer explicit versioned directories or manifests over heuristic discovery.

## Versioning and compatibility

- The native core version is the canonical compatibility key.
- Desktop and SDK artifacts must declare the exact compatible native core version or compatible ABI range.
- A desktop bundle must fail fast on ABI mismatch rather than silently downgrade behavior.
- Patch releases may preserve ABI compatibility, but the release manifest must still declare the specific tested native core build.

## Validation and testing rules

- Every release must include a native validation path that does not require the Avalonia shell.
- Packaging verification must confirm that the desktop bundle resolves the intended native core and reports mismatch failures clearly.
- Release checks should cover artifact presence, ABI compatibility, startup smoke tests, and manifest consistency.
- Packaging must preserve the ability to test the native layer in CI without a desktop session.

## Error behavior

Stable error families for packaging and distribution:

- `CORTEX_ERR_PACKAGE_CORE_MISSING`
- `CORTEX_ERR_PACKAGE_ABI_MISMATCH`
- `CORTEX_ERR_PACKAGE_HOST_BOOTSTRAP_FAILED`
- `CORTEX_ERR_PACKAGE_MANIFEST_INVALID`
- `CORTEX_ERR_PACKAGE_PLATFORM_UNSUPPORTED`

Each failure should report:

- the failing artifact class
- the expected and actual core or ABI version when relevant
- the platform target
- whether the failure is retryable by reinstall, rebuild, or repackaging

## Compatibility and versioning notes

- This packaging model is Windows-first, but it should not hardcode Windows-only assumptions into artifact identity or manifest structure.
- Additional future package classes may be added if they preserve native-core authority and explicit version matching.
- The packaging model should remain compatible with future external integration lanes such as `#3`, which will depend on stable host and ABI identity.

## Example usage

```text
1. A release build produces a native core library, a native host executable, and an Avalonia desktop bundle.
2. The release manifest declares native core version 0.4.0 and ABI version 3.
3. A validation step runs the native host directly and confirms lifecycle and entity queries work without the desktop shell.
4. The desktop bundle starts, verifies that the bundled native core also reports version 0.4.0 and ABI version 3, then opens normally.
5. If the shell finds ABI version 2 instead, it fails fast with an explicit package compatibility error instead of trying to run.
```
