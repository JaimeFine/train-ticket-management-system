---
name: project-collaboration-operating-system
description: Use when redesigning or improving AI-native engineering collaboration for an existing project, especially around documentation systems, local context, staged scope, correction loops, retrospectives, decision records, and verification habits
---

# Project Collaboration Operating System

## Overview

Use this skill to make an existing project easier for AI agents and humans to develop together. Do not copy another repository's docs. Extract the operating habits: how agents find authority, control scope, preserve local context, prove work, and correct themselves when they drift.

Core principle: AI-native project documentation is not a knowledge dump. It is a control system for repeated agent failure modes: shallow context, overbuilding, fake success, skipped verification, local draft pollution, and poor correction after feedback.

## Invocation Template

When the user needs a copyable prompt for invoking this skill, read `templates/invocation.md`. Keep the template as an entry aid, not a replacement for this skill's operating rules.

## Non-Negotiables

- Identify authoritative context before acting.
- Do not treat local notes, drafts, screenshots, or old audits as formal rules.
- Do not show user-visible success without a traceable result or explicit placeholder.
- Do not expand project scope, dependencies, infrastructure, or test systems without a confirmed stage decision.
- Do not claim completion without project-appropriate evidence.
- When behavior violates project rules, classify the failure before changing docs. Correct agent-only misses immediately; improve docs or workflow when the system made the miss likely.
- A retrospective must change the next action. "Be careful next time" is not a fix.

## Layer 1: Build The Collaboration System

Design only the collaboration surfaces the target project actually needs.

1. **Inventory context**
   - Find project entrypoints, agent instructions, coding rules, product/domain constraints, design or architecture notes, scripts, test docs, local notes, decision records, ignored artifact paths, and module guidance.
   - If something is missing, mark it as a gap. Do not invent authority.

2. **Classify authority**
   - Separate binding rules from explanatory docs, temporary notes, generated plans, old audits, screenshots, and speculative ideas.
   - Define precedence: user instruction, repository agent rules, formal docs, decision records, local notes, code reality.

3. **Define stage boundaries**
   - Record what the project is trying to accomplish now.
   - Name capabilities that must not be added casually: new infrastructure, auth, databases, AI services, CI, UI systems, test frameworks, or other large surfaces.

4. **Define behavior contracts**
   - For actions such as save, submit, collect, apply, invite, edit, delete, or publish, require a write path: state, storage, service, API, event, queue, or explicit staged placeholder.
   - Forbid success copy that has no traceable result.

5. **Define local-memory policy**
   - Use ignored or clearly labeled local space for AI summaries, task plans, exploratory audits, screenshots, temporary test reports, and unconfirmed ideas.
   - Local memory helps continuity; it does not override formal rules.

6. **Define verification contract**
   - Connect change types to existing checks: lint, typecheck, tests, build, smoke checks, browser checks, server health, format checks, or manual QA.
   - Do not add a new framework just to satisfy process. First use the project's checks; if missing, propose the smallest useful guardrail.

7. **Use methodology skills when available**
   - If Superpowers or a similar skill library exists, route work to the right process: brainstorming for unclear work, systematic debugging for failures, TDD for implementation, planning for execution design, verification before completion, code review for quality, writing-skills for skill work.
   - If no such skills exist, follow the equivalent method manually.
   - Keep this as a routing rule, not the center of the skill.

## Layer 2: Run The Iteration Loop

The collaboration system gets better through accurate failure classification, not blame or constant document rewriting. Some failures are agent execution misses. Others prove that the project docs, context map, or verification process made the wrong behavior too easy.

Use this loop when the agent violates project docs, the user says the output missed the documented collaboration style, verification fails, or the same mistake appears across sessions.

1. **Detect drift**
   - Examples: read too little, treated local notes as policy, overbuilt, changed shared surfaces for local needs, skipped verification, showed fake success, ignored a stage boundary.

2. **Stop and name the miss**
   - State the rule or expectation missed.
   - Do not defend intent. The useful question is what happened and what must change.

3. **Classify the failure before changing docs**

| Failure type                    | Signal                                                                    | Action                                                            |
| ------------------------------- | ------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| Agent execution failure         | The rule was clear, the context existed, and the agent ignored it.        | Correct behavior. Do not change docs.                             |
| Context discovery failure       | The information existed, but the agent failed to find or read it.         | Improve the context map, reading strategy, or agent instructions. |
| Documentation ambiguity failure | The docs existed, but allowed multiple reasonable interpretations.        | Clarify the rule, precedence, examples, or anti-examples.         |
| Documentation gap failure       | The project had no rule for an important or repeated situation.           | Create durable guidance, a decision record, or a checklist.       |
| Process failure                 | The project had no verification, feedback, ownership, or correction loop. | Improve the workflow and required checks.                         |

4. **Correct the work**
   - Fix the deliverable first using the current authoritative rule.
   - If the failure is systemic, propose the smallest doc or process change that would prevent recurrence.
   - Do not edit formal team docs unless the project allows it or the user confirms the change.

5. **Retrospect briefly**
   - Use this shape:

```text
Miss:
Failure type:
Root cause:
Corrected action:
System change: none / context map / doc clarification / new guidance / workflow
Prevention:
```

## Artifact Selection

Choose artifacts by pain, not by copying a template.

- AI does not know what to read: add agent instructions or a context index.
- Existing guidance is hard to find: improve the context map, reading order, or agent entrypoint.
- Existing guidance causes different interpretations: add precedence rules, examples, or anti-examples.
- Code placement is inconsistent: add a coding guide.
- Agents open too much scope: add stage boundaries and deferred-capability rules.
- User-visible actions fake success: add behavior contracts.
- Local drafts pollute shared rules: add a local-notes policy.
- Shared choices are forgotten: add a decision log.
- Completion is claimed without proof: add a verification guide.
- Feedback does not change future behavior: add a correction loop or retrospective format.
- The same AI mistake repeats: add a correction protocol, local checklist, or decision note.

Some projects need one compact document. Others need several files. Fit the project's maturity.

## New Project Diagnosis

Before proposing changes in a target project, produce a compact diagnosis:

```text
Authoritative context:
Temporary/local context:
Current scope gates:
Behavior/write-path rules:
Verification contract:
Correction loop:
Known failure types:
AI collaboration gaps:
```

Then propose the minimum redesign:

- Existing docs to update.
- Missing guidance to add.
- Local notes that must remain local.
- Decisions that need durable records.
- Checks agents must run after each change type.
- Correction habit that prevents repeated drift.

## Pressure Checks

Use these scenarios to test whether the collaboration system works:

- **README-only shortcut:** An agent reads one entry file and starts editing.
- **Draft-policy confusion:** A local plan conflicts with formal docs.
- **Fake-success shortcut:** An agent shows "saved", "submitted", or "done" without a write path.
- **Infrastructure temptation:** An agent adds a database, auth, new library, test framework, AI service, or CI without a stage decision.
- **No-verification finish:** An agent summarizes confidently without running or reporting checks.
- **User correction:** The user says the agent ignored docs or missed the intended collaboration style.
- **Ambiguous-doc fork:** Two agents interpret the same rule differently and both can justify their reading.
- **Process-hole repeat:** Agents repeatedly miss verification or feedback because no workflow requires it.

## Common Mistakes

| Mistake                                      | Fix                                                                                              |
| -------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| Copying a successful project's docs verbatim | Extract habits and redesign for the target project's scale.                                      |
| Making a documentation taxonomy              | Tie each document to an agent action, gate, or verification.                                     |
| Treating local notes as policy               | Define authority and promotion rules.                                                            |
| Apologizing without changing execution       | Name the miss, fix the work, and state prevention.                                               |
| Editing team docs as a reflex                | Classify the failure first; change docs only for discovery, ambiguity, gap, or process failures. |
| Blaming the agent when docs mislead agents   | Fix the context map, wording, examples, or workflow that caused the miss.                        |
| Calling a retro done without an action       | Add a concrete changed behavior, owner, or guardrail.                                            |

## Completion Checklist

- The result is project-neutral if meant for reuse.
- The target project is not forced into another project's file names, tools, or workflow.
- Formal rules, local notes, decisions, verification, and correction each have clear roles.
- AI failure modes are addressed with concrete gates.
- Failure classification distinguishes agent mistakes from docs or process defects.
- The artifact set is minimal for the project's maturity.
- Future agents can recover context and know how to correct drift.
