# CORTEX Runtime Topology For Child Programs

Issue alignment: `CORTEX #7`

## Purpose

Define the canonical runtime topology for `CORTEX` child programs under a native C-first architecture.

The runtime core remains native and authoritative. Avalonia and C# may host or present CORTEX state where needed, but they do not become a second runtime or domain authority.

This contract assumes the entity model in [`canonical-entity-model.md`](./canonical-entity-model.md), the lifecycle rules in [`lifecycle-state-machine.md`](./lifecycle-state-machine.md), and the stack rule in [`native-c-avalonia-direction.md`](./native-c-avalonia-direction.md).

## Interface contract

The runtime topology must be expressed through four explicit layers:

- `cortex_core`: the canonical native C core library that owns entity state, lifecycle rules, governance logic, and validation
- `cortex_native_host`: native child programs or native host processes that link to `cortex_core` directly for default execution
- `cortex_desktop_host`: an optional Avalonia desktop shell that consumes the native layer through an explicit C ABI and remains presentation-only
- `cortex_runtime_bridge`: the narrow ABI or process boundary contract used when a managed host, isolated worker, or external process must invoke native CORTEX behavior

No runtime layer outside `cortex_core` may redefine lifecycle, entity ownership, or component-governance rules.

## Expected inputs and outputs

Inputs into the runtime topology:

- canonical entity and lifecycle descriptors from `cortex_core`
- operator action requests such as validate, mark ready, activate, suspend, or archive
- host capability metadata such as `native-only`, `desktop-shell`, or `isolated-worker`
- platform and packaging signals that determine whether work should stay in process or cross a process boundary

Outputs from the runtime topology:

- lifecycle snapshots and legal-action descriptors returned from `cortex_core`
- governed diagnostics for native child startup, host compatibility, and interop failures
- explicit handles or references for any child program, worker, or shell session created by the runtime
- stable ABI responses that let managed hosts render state without owning it

## Topology rules

### Native-first execution

- `cortex_core` must be usable without any managed host present.
- Native child programs are the default execution path unless a desktop shell is specifically required.
- Runtime behavior that defines the character, component array, or lifecycle state must stay in native code.

### Desktop host role

- The Avalonia desktop host may present state, route operator actions, and render project or character views.
- The desktop host must call into the native core through a stable C ABI or a clearly defined bridge.
- The desktop host may not cache or mutate canonical state independently of the native core.

### Child program classes

- `native-ui-host`: an optional native host for direct platform-bound execution without managed UI
- `native-worker`: background or task-specific child programs that execute native CORTEX work directly
- `desktop-shell`: an Avalonia or C# presentation host over the native layer
- `isolated-worker`: a process boundary used only when isolation, recovery, or packaging constraints justify it

## Process boundary rules

- In-process native execution is the default topology.
- A process boundary is justified only when isolation, crash containment, privilege separation, or packaging rules require it.
- Crossing a process boundary must preserve explicit lifecycle ownership and error reporting.
- IPC or bridge traffic must carry explicit request and response contracts; hidden state transfer is not allowed.

## Interop and memory ownership

- The managed host must talk to native CORTEX through a narrow, versioned C ABI.
- Native CORTEX owns the canonical memory and lifecycle of runtime entities unless an exported contract explicitly says otherwise.
- Returned native buffers or handles must have explicit release rules.
- Managed wrappers may adapt native responses for UI rendering, but they must not become shadow authorities for runtime state.

## Error behavior

Stable native error families for this topology:

- `CORTEX_ERR_ABI_VERSION_MISMATCH`
- `CORTEX_ERR_HOST_CAPABILITY_MISMATCH`
- `CORTEX_ERR_RUNTIME_BOUNDARY_VIOLATION`
- `CORTEX_ERR_PROCESS_TOPOLOGY_UNSUPPORTED`
- `CORTEX_ERR_NATIVE_CHILD_BOOTSTRAP_FAILED`

Each failure should report:

- the failing host or child-program class
- the failing boundary or ABI version
- the native rule id or topology constraint
- whether the failure is retryable in process, out of process, or only after rebuild or repackage

## Compatibility and versioning notes

- The runtime topology contract is Windows-first, but it must not bake Windows-only assumptions into the core ABI.
- New host classes may be added later if they preserve native-core authority and explicit bridge contracts.
- Packaging decisions remain downstream of this topology contract and should refine deployment, not rewrite runtime ownership.
- The native ABI version must be exposed explicitly to managed hosts and isolated workers.

## Example usage

```text
1. The operator opens the Avalonia desktop shell for CORTEX.
2. The desktop host loads `cortex_core` through the stable native bridge and requests the selected character snapshot.
3. The operator chooses `validate_character`.
4. The desktop host sends the request through the ABI to the native core.
5. The native core performs validation, returns the updated lifecycle snapshot, and starts a native worker only if isolation is required for a follow-on task.
6. The shell renders the returned native state without taking ownership of lifecycle or entity logic.
```
