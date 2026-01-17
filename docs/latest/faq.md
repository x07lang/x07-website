# FAQ

## Is X07 a general-purpose language?

Yes—via OS worlds it targets general-purpose usage (CLI tools, servers, automation).

Fixture worlds exist to make testing and repair deterministic, not to limit real-world capability.

## Why not just use Rust / Go / Python?

You can.
X07 exists because autonomous agents struggle with:

- multiple equivalent patterns,
- ambiguous diagnostics,
- nondeterministic test environments,
- text-based patching on fragile syntax.

X07 makes those constraints first-class.

## Is X07 safe?

In deterministic worlds, X07 is designed to be safe-by-construction.
In OS worlds, safety depends on policies and host sandboxing.

## Is X07 fast?

X07 compiles to C and can call native shims for performance-critical workloads.

The main performance risk is agent-written O(n²) logic—so the ecosystem includes budgets, metrics, and canonical builders.

## Where do I download X07?

Download the toolchain archives (macOS/Linux/Windows) and the optional skills pack from GitHub releases:

- https://github.com/x07lang/x07/releases

See [Install X07](getting-started/install.md).
