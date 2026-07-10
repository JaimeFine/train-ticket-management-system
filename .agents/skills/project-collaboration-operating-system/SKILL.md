---
name: project-collaboration-operating-system
description: Use when bootstrapping, auditing, improving, or retrospecting on human-AI collaboration systems in a project, especially around context entrypoints, authority, staged scope, local drafts, verification, decision records, documentation drift, and correction loops.
---

# Project Collaboration Operating System

## Overview

Use this skill to govern a project's human-AI collaboration system. It makes collaboration context discoverable, authority clear, scope controlled, completion verifiable, and drift correctable.

This is not a product strategy skill, PRD generator, roadmap planner, market research workflow, or full lifecycle advisor.

Use `templates/invocation.md` only when the user needs a copyable invocation prompt.

## Thinking Principle

Treat the user's proposal and your own first judgment as hypotheses, not conclusions.

Look for evidence that weakens the current judgment before strengthening it.

Revise the judgment when evidence changes; change the collaboration system only when evidence justifies a durable change.

## Core Operating Model

Run this exact chain for every mode:

```txt
Select mode
-> Context
-> Authority
-> Scope
-> Diagnosis
-> Minimal Change
-> Verification
-> Correction
```

- **Select mode:** Route the request to Bootstrap, Audit, Improve, or Retrospective.
- **Context:** Inspect entrypoints, docs, code reality, local notes, known decisions, uncommitted work, and the user's latest instruction.
- **Authority:** Separate binding rules, explanatory docs, temporary notes, stale content, inferred ideas, and suggestions.
- **Scope:** Confirm the active stage and deferred capabilities.
- **Diagnosis:** Classify the need or failure before changing docs or process.
- **Minimal Change:** Prefer clarification, links, deduplication, status marking, or authority cleanup over new documents.
- **Verification:** Check that the intended future behavior is discoverable and evidence-backed.
- **Correction:** Preserve only durable learning in formal docs, decision records, local context policy, or workflow.

## Route To A Mode

Route first, then run the core operating model:

| Signal                                                                                                                                                    | Mode            |
| --------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------- |
| No stable collaboration context, repository entrypoint, or agent boundary exists; input is mostly ideas, chats, screenshots, rough notes, or partial code | `bootstrap`     |
| Existing project/docs need diagnosis or the user asks what the current collaboration system looks like                                                    | `audit`         |
| A known collaboration gap, documentation problem, or workflow upgrade is requested                                                                        | `improve`       |
| A real miss, drift, repeated failure, skipped verification, fake completion, or correction event occurred                                                 | `retrospective` |

If facts are mixed with guesses or the active stage is unclear, ask only the next highest-impact clarification needed for collaboration governance.

## Non-Negotiables

- Do not promote inferred ideas, guesses, local notes, drafts, screenshots, old audits, generated plans, or historical reports into formal rules.
- Do not create a complete documentation suite by default; choose artifacts by collaboration pain and project maturity.
- Do not hide an agent execution failure with documentation edits.
- Do not show user-visible success without a traceable result or explicit staged placeholder.
- Do not claim completion without project-appropriate evidence.
- Do not expand scope, dependencies, infrastructure, auth, databases, AI services, UI systems, CI, or test frameworks without a confirmed stage decision.
- Do not let duplicated guidance drift silently; identify the source of truth and sync, link, or remove stale copies.
- Do not overwrite or revert uncommitted user work.
- Do not copy personal AI skill directories, external local skill folders, or another repository's docs into the project.
- Classify the issue before changing the collaboration system.
- A retrospective must change the next action; "be careful next time" is not a fix.

## Mode: Bootstrap

Use Bootstrap when stable collaboration context does not exist. Create the smallest usable human-AI collaboration system, not complete product definition.

Mode-specific actions:

1. Gather available material: repo reality, chats, screenshots, notes, rough ideas, existing docs, and latest intent.
2. Sort material into confirmed facts, inferred ideas, open questions, deferred items, and non-goals.
3. Define only the context, authority surfaces, scope gates, and verification expectations needed for the next collaboration round.
4. Mark unresolved assumptions visibly.
5. Read `references/bootstrap.md` only if messy input needs heavier sorting or a v0 document-role sketch.

Typical output: confirmed collaboration facts, open questions, current scope gates, minimal document roles, immediate risks, and next safest action.

## Mode: Audit

Use Audit when docs or a repository already exist and the user needs the current collaboration state understood.

Mode-specific actions:

1. Inventory entrypoints, agent instructions, coding rules, product/domain constraints, decision records, local notes policy, scripts, verification docs, and code reality.
2. Classify authority: binding rules, explanatory docs, temporary notes, generated plans, historical audits, screenshots, speculative ideas, and stale content.
3. Check duplication and drift across rules, routes, commands, scope gates, behavior contracts, and verification requirements.
4. Check user-visible behavior contracts: save, submit, collect, like, follow, apply, invite, edit, delete, publish, and similar actions need a write path or explicit placeholder.
5. Check whether verification expectations match source, docs, routes, scripts, backend, or behavior changes.

Typical output: authoritative context map, temporary/local context map, active scope gates, behavior/write-path rules, verification contract, correction loop status, duplicated guidance/drift risks, and AI collaboration gaps.

## Mode: Improve

Use Improve when a collaboration gap or upgrade target is known.

Mode-specific actions:

1. Decide whether the issue is an agent execution failure or a system failure.
2. If rules are clear and the miss is execution-only, correct execution and do not change formal docs.
3. If the system failed, change the smallest authority surface: clarify, link, deduplicate, mark status, add an anti-example, update a decision note, or adjust workflow.
4. Avoid adding a whole new document, framework, or process for a local problem.
5. Report what changed, why it is the minimum needed, and what was intentionally left unchanged.

Typical output: failure classification, target surface, minimal change plan or edit summary, affected docs, verification/follow-up checks, and residual risks.

## Mode: Retrospective

Use Retrospective when a real miss, drift, repeated error, verification failure, or false completion statement happened. Do the Failure Review before deciding whether to change the system.

Required shape:

```txt
Miss:
Expected rule:
Failure type:
Root cause:
Immediate correction:
System change:
Prevention:
```

Failure type must be one of:

```txt
agent execution
context discovery
documentation ambiguity
documentation drift
documentation gap
process failure
```

Mode-specific actions:

1. Stop and name the miss without defending intent.
2. Identify the expected rule or missing rule.
3. Classify the failure type before changing docs.
4. Correct the actual work first.
5. If existing rules were clear, change execution only.
6. If the failure is systemic, make the smallest change to authoritative docs, decision records, local context policy, or workflow.

Typical output: Failure Review, corrected action, system change decision, verification evidence or skipped-verification rationale, and prevention note.

## Artifact Selection

Choose artifacts by collaboration pain, not by copying a template. These are roles, not mandatory file names:

- **README:** human project entry, current stage, quick start, route/doc index.
- **Agent instructions:** AI entrypoint, read-first order, scope gates, verification, correction triggers.
- **Coding guide:** code placement, naming, style, data access, behavior implementation rules.
- **Collaboration guide:** team workflow, authority map, local context policy, correction loop, decision-record triggers.
- **Decision notes:** durable decisions that affect multiple collaborators, shared architecture, scope, global styles, data contracts, or verification.
- **Verification guide:** specific repeatable verification workflow when the project needs it.
- **Local notes policy:** ignored workspace for drafts, AI summaries, screenshots, temporary plans, and unconfirmed ideas.

Use these pain-to-artifact mappings:

- AI does not know what to read: add or clarify the agent entrypoint/context index.
- Guidance exists but is hard to find: improve read order, links, or authority map.
- Guidance allows multiple reasonable interpretations: clarify wording, precedence, examples, or anti-examples.
- Guidance is duplicated: choose a source of truth and reduce, link, or sync secondary copies.
- Code placement is inconsistent: add or tighten a coding guide.
- Agents open too much scope: add stage boundaries and deferred-capability rules.
- User-visible actions fake success: add behavior/write-path contracts.
- Local drafts pollute formal rules: add local context policy.
- Shared choices are forgotten: add decision notes.
- Completion is claimed without proof: add verification contract.
- Feedback does not change behavior: add correction loop.

## Verification Contract

Verification is about changed behavior, not document volume. Match checks to the changed surface:

- A future agent can discover the correct entrypoint and read order.
- Authority between formal docs, decision records, local drafts, historical audits, and code reality is clear.
- Duplicate rules were removed, linked, or intentionally kept in sync.
- Active scope and deferred capabilities are visible.
- User-visible success has a store, service, API, event, queue, or explicit placeholder path.
- Completion claims require evidence appropriate to the project and change type.
- Local drafts, screenshots, temporary plans, and old audits cannot reasonably be mistaken for current rules.
- If verification was not run, the reason and residual risk are stated.

Do not add a new verification framework merely to satisfy process. Use existing project checks first; if checks are missing, propose the smallest useful guardrail.

## Common Failure Patterns

| Failure pattern                                                 | Correction                                                           |
| --------------------------------------------------------------- | -------------------------------------------------------------------- |
| Treating an execution miss as a documentation gap               | Classify failure first; do not change docs when rules were clear.    |
| Adding rules whenever anything goes wrong                       | Fix the work first; add rules only for durable system gaps.          |
| Growing document count while authority gets less clear          | Consolidate, link, or assign source of truth.                        |
| Treating historical audits as current facts                     | Mark status, archive, or route them as evidence only.                |
| Treating local drafts as formal policy                          | Clarify local context policy and promotion rules.                    |
| Writing plans, placeholders, or staged UI as completed behavior | Require write paths or explicit placeholder copy.                    |
| Introducing a full new system for a local problem               | Make the smallest justified intervention.                            |
| Copying another project's docs verbatim                         | Extract operating habits and adapt to the target project's maturity. |
| Overwriting user work while improving docs                      | Inspect worktree state and preserve unrelated changes.               |
| Using method names instead of concrete actions                  | State the required action, evidence, and authority surface.          |

## Completion Checklist

- The selected mode matches the user's intent and available collaboration context.
- Formal context and temporary/local context are identified separately.
- The need or failure has been classified before system changes.
- The change or recommendation is the smallest necessary intervention.
- Repeated or conflicting authority has a clear source of truth.
- Verification matches the changed surface.
- Unrun verification and residual risk are stated.
- User-visible behavior rules require traceable results or explicit staged placeholders.
- Only durable project learning enters formal docs or decision records.
- Reusable guidance does not force another project into this project's file names or workflow.
