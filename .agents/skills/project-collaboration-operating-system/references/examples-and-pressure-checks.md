# Examples And Pressure Checks

This file is for skill maintenance, examples, and forward-testing. It is not required for ordinary use.

## TripKin Evolution Case

TripKin should be used as an evolution case, not a template to copy.

High-level progression:

1. Initial engineering shell and architecture/setup drafts established a runnable mobile H5 project and rough collaboration boundaries.
2. The first formal collaboration system separated roles: project entry, agent instructions, coding guide, collaboration guide, and decision notes.
3. Shared component and global style rules were tightened after collaboration risk appeared around common surfaces.
4. Product PRD, demo fix scope, component-library guidance, and local verification guidance were added as the project matured.
5. Backend/API boundaries and service access rules were introduced only when the frontend needed a staged handoff path.
6. The project shifted from page demo work to MVP closure, adding behavior write-path rules and user asset continuity.
7. Failure Review, local context policy, source-of-truth handling, and documentation drift checks were added after AI collaboration drift became visible.

Reusable lesson: extract document roles, stage gates, behavior contracts, verification rules, and correction loops. Do not copy TripKin-specific routes, product stage, UI stack, or travel-domain behavior into unrelated projects.

## Pressure Scenarios

Use these as test assertions after substantial edits.

### 1. Zero-doc idea dump

Prompt:

```txt
I want to build an AI travel companion app, kind of like Xiaohongshu plus maps plus MBTI. I have no PRD. Please set up the collaboration docs.
```

Expected behavior:

- Select Bootstrap.
- Do not generate a complete PRD or roadmap.
- Classify confirmed facts, inferred ideas, open questions, deferred items, and non-goals.
- Propose minimal collaboration context and scope gates.

### 2. Existing docs but unclear authority

Prompt:

```txt
This repo has README, AGENTS, a product audit, and local notes. Agents keep following old drafts. Audit the collaboration system.
```

Expected behavior:

- Select Audit.
- Separate formal docs, local drafts, historical audits, and code reality.
- Identify source-of-truth gaps and drift risk.
- Propose minimal improvements.

### 3. Known gap after one execution miss

Prompt:

```txt
An agent ignored the existing verification rule and said done without running checks. Update the docs so this never happens again.
```

Expected behavior:

- Select Retrospective or Improve depending on whether a Failure Review is requested.
- Classify as agent execution if the rule already existed and was clear.
- Do not add process if execution correction is enough.

### 4. Template-copy temptation

Prompt:

```txt
Copy TripKin's docs into my new project because they look complete.
```

Expected behavior:

- Reject direct copying.
- Extract roles and operating habits.
- Ask for or inspect target project context before creating project-specific rules.

### 5. Local draft pollution

Prompt:

```txt
The local screenshot notes say to redesign the UI, but README says current scope is backend closure. What should the agent follow?
```

Expected behavior:

- Treat local notes as non-authoritative unless explicitly promoted.
- Follow formal docs and current user instruction.
- Recommend status marking only if ambiguity is systemic.

## Maintenance Self-Check

After editing the skill, verify:

- Bootstrap does not invade Audit, Improve, or Retrospective.
- Router is an entry selector, not the core chain.
- Core operating model remains in `SKILL.md`.
- Requirement shaping, planning, template extraction, and case generalization are not independent modes.
- References contain heavy optional guidance only.
- TripKin appears as an evolution case, not a universal template.
