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
Canonical X07 source is `x07AST` JSON, but reading raw JSON is rough on the eyes. Below is the same module in x07text, the lossless S-expression projection you get from `x07 ast to-text`. It parses straight back to the JSON on disk.
:::

```clojure
; x07text
{
  :kind module
  :module_id app.core
  :schema_version x07.x07ast@0.8.0
  :imports ()
  :decls ({:kind defn :name app.core.normalize_path_v1 :body (app.core.trim_path_v1 path) :params ({:name path :ty bytes_view}) :requires ({:expr (> (view.len path) 0) :id non_empty}) :result bytes}
  )
}
```

One pure function, `normalize_path_v1`: it borrows a byte view, returns owned bytes, and carries a named contract clause (`non_empty`) that says the input must hold at least one byte. The real work is delegated to a helper. The point isn't the function — it's that the boundary is written down where a human or an agent can both see it.

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

I do not think coding-agent reliability is only a model problem. Better models, better prompting, and better tools all help. But underneath that there is a structural mismatch: the way agents generate code does not match the way most languages expect code to be produced, edited, and verified.

So I keep coming back to one idea:

**if agents are going to write a large share of the code, the language should expose explicit contracts at the points where failure is most expensive.**

X07 is my attempt at building that idea all the way through: the language, the toolchain, the MCP kit, the WASM target, and the review and trust artifacts that let you check the result.

It is still under active development. APIs will change, and some surfaces are far more mature than others. But the direction is deliberate, and the docs describe a stack that actually runs, not a slide-deck thesis.

If you keep hitting the wall where agents produce code that is plausible but not dependable, the model is only half the question. The other half is what kind of language and toolchain you are asking that model to work inside.
