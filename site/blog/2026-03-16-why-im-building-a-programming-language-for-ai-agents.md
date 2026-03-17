---
slug: why-im-building-a-programming-language-for-ai-agents
title: Why I'm Building a Programming Language for AI Agents
description: X07 started as a response to quiet coding-agent failures and turned into a broader bet on explicit software contracts.
tags: [x07, founder story, programming languages, agentic coding]
---

I have spent a lot of time watching coding agents fail in quiet ways.

Not the obvious failures. Not the "I cannot do that" failures.

The quiet ones.

The function that looks clean but uses the wrong boundary encoding. The patch that compiles but drifts away from the repo's architecture. The generated test that passes because the fixture is too weak, not because the code is right.

Those failures changed how I think about programming languages.

<!-- truncate -->

## The moment it clicked

The turning point was realizing that a lot of agent mistakes are not really syntax failures or model-intelligence failures.

They are ambiguity failures.

The model sees several valid ways to do the same thing, chooses one that is statistically common, and ends up wrong for the actual codebase.

That leads to a practical question:

**what would a language look like if it were designed for the agent, not for the human typist?**

That question turned into X07.

## What I wanted the language to do differently

I did not want to bolt an "AI mode" onto a normal language.

I wanted the core workflow to assume:

- structured source instead of fragile text edits
- explicit capability boundaries instead of hidden side effects
- deterministic replay instead of flaky debugging
- machine-readable diagnostics instead of prose-only error messages
- local budgets instead of unbounded execution

That is why X07's canonical source form is [x07AST JSON](/docs/language/syntax-x07ast), why worlds are explicit in the [world model](/docs/worlds), and why the [repair loop](/docs/toolchain/repair-loop) is built into the normal toolchain path.

## What that looks like in code

:::note
This example is based on the X07 language itself, using canonical `x07AST` JSON. The comments explain the structure because many readers will not be familiar with X07 yet.
:::

```jsonc
{
  "kind": "defn", // Define one pure X07 function.
  "name": "app.core.normalize_path_v1", // Use a stable module-qualified symbol name.
  "params": [
    {"name": "path", "ty": "bytes_view"} // Accept a borrowed byte-view input.
  ],
  "result": "bytes", // Return owned normalized bytes.
  "requires": [
    {
      "id": "non_empty", // Name the contract clause for diagnostics and review.
      "expr": [">", ["view.len", "path"], 0] // Require the input path to contain at least one byte.
    }
  ],
  "body": ["app.core.trim_path_v1", "path"] // Delegate the real work to another pure helper.
}
```

That snippet is tiny, but it captures the design goal:

make the constraints explicit enough that both a human and an agent can see the intended boundary.

## What surprised me during the work

Three things stood out.

### 1. The same features help humans too

I started from agent reliability, but the same properties make human review easier:

- fewer equivalent patterns
- more replayable failures
- clearer boundaries
- less guesswork in diagnostics

The things that help machines reason locally also reduce human cognitive load.

### 2. Local-model generation became much more plausible

Because X07 uses structured source, the toolchain can export a schema and grammar bundle for constrained decoding through [Genpack](/docs/genpack/).

That means local models can target "valid X07 source structure" directly instead of trying to imitate a text syntax from scratch.

### 3. Agent-first design did not have to mean toy performance

The published [`x07-perf-compare`](https://github.com/x07lang/x07-perf-compare) repo is useful here because it keeps the performance story inspectable.

In the February 9, 2026 macOS snapshot in that repo, X07 direct-binary runs landed near C and Rust on the included workloads, while warm-cache compile times came in about `1.3x-1.8x` faster than C and `3.1x-3.7x` faster than Rust, with binaries around `34 KiB`.

That is one snapshot on one machine, not a universal law. But it is enough to reject the idea that an agent-first language has to be slow, bloated, or purely academic.

## The broader bet

I do not think the coding-agent reliability problem is only a model problem.

Better models clearly matter. Better prompting matters. Better tools matter.

But there is also a structural mismatch between how agents generate code and how most languages expect code to be produced, edited, and verified.

That is why I keep coming back to a simple idea:

**if agents are going to write a large share of the code, the language should expose explicit contracts at the points where failure is most expensive.**

X07 is my attempt at building that idea all the way through:

- language
- toolchain
- MCP kit
- WASM and device flows
- review and trust artifacts

It is still under active development. APIs will change. Some surfaces are much more mature than others. But the direction is intentional, and the current docs already describe a real working stack rather than a slide-deck thesis.

If you are hitting the wall where coding agents produce code that is plausible but not dependable, I think the right question is no longer just "which model should I use?"

It is also:

**what kind of language and toolchain am I asking that model to work inside?**
