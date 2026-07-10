# Bootstrap Reference

Read this only when Bootstrap mode is selected and messy input needs extra sorting or a v0 document-role sketch.

## Input Classification

Sort messy input before drafting rules:

| Bucket          | Meaning                                                                            | Treatment                                                      |
| --------------- | ---------------------------------------------------------------------------------- | -------------------------------------------------------------- |
| Confirmed facts | Explicitly stated by the user, visible in the repo, or backed by current docs/code | May become formal context.                                     |
| Inferred ideas  | Reasonable interpretation from conversations, screenshots, or code                 | Mark as inference; ask before formalizing if high impact.      |
| Open questions  | Unknowns that block scope, authority, or verification                              | Ask the next highest-impact question only.                     |
| Deferred items  | Useful later but not needed for the current stage                                  | Keep out of current scope gates except as deferred capability. |
| Non-goals       | Explicitly out of scope or unsafe to open casually                                 | Put in scope gates when durable.                               |

## Lightweight Interview

Ask only what is needed for collaboration governance.

Use these questions when the answer cannot be discovered from the repo or supplied material:

1. What current project or task boundary must future agents respect?
2. What is explicitly not in scope right now?
3. What result would count as real completion rather than a staged placeholder?
4. Which facts are confirmed, and which are only ideas or guesses?
5. Which decisions should persist for future collaborators?
6. Is there existing code, documentation, or uncommitted work that must be protected?

If multiple questions remain, ask the one that most affects authority, scope, or verification first.

## Minimal Collaboration Context

For zero-doc projects, prefer a compact first context:

- Project entry: what the project is, how to run or inspect it, and where the current truth lives.
- Agent boundary: what to read first, what not to do, and what evidence is needed before claiming completion.
- Scope gate: current stage and deferred capabilities.
- Placement rules: where code, docs, data access, shared state, and temporary notes should live.
- Verification rule: checks or manual evidence appropriate for the current project.
- Decision memory: where durable cross-collaborator decisions go.

These may live in one document at first.

## V0 Document Roles

Use roles, not fixed templates:

- README-like role: human entrypoint and project status.
- Agent-instructions role: AI read order, scope gates, verification, and correction triggers.
- Coding-guide role: implementation placement and naming rules.
- Collaboration-guide role: workflow, local context policy, decision triggers, and correction loop.
- Decision-note role: durable decisions that affect multiple collaborators.

## Bootstrap Differences By Maturity

- **Only ideas/chats:** produce confirmed facts, inferred ideas, open questions, non-goals, and a first collaboration brief.
- **Partial code, no docs:** inspect code reality, define entrypoint and scope gates, then add minimal agent/coding guidance.
- **Template README only:** replace template claims with current project facts; do not invent product strategy.
- **Existing docs but unstable:** switch to Audit mode after establishing entrypoints and authority.

## Bootstrap Output Shape

```txt
Mode: bootstrap
Confirmed collaboration facts:
Inferred or unconfirmed items:
Open questions:
Current scope gates:
Minimal document roles:
Verification expectation:
Next safest action:
```
