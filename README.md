# CORTEX

CORTEX is the canonical management surface for agents, subagents, arrays, roles, lifecycle actions, and personality-aware control inside SYMBIOSIS and SYNAPSE.

## Direction

- Native C is the primary implementation language for the CORTEX core.
- Avalonia is the preferred desktop shell where a managed UI surface is required.
- C# is used only where Avalonia, interop hosting, or platform packaging makes it necessary.
- Canonical behavior stays in Native C behind an explicit, stable interop boundary.

## Architecture rule

The Native C core owns entity modeling, lifecycle behavior, array ownership, role attachment, and management logic. Avalonia and C# remain thin host layers rather than a second runtime authority.

See [docs/native-c-avalonia-direction.md](docs/native-c-avalonia-direction.md) for the current stack rule and packaging implications.
