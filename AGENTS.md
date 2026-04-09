# AGENTS.md

This repository is used as a general Unreal Engine workspace. Follow these rules even if the current project name changes later.

## Scope

These instructions apply to the entire repository tree from this file downward.

## Working Directory and Project Discovery

Do not assume the agent starts at the repository root.

Before making changes:
- Find the repository root by locating `.git` or this `AGENTS.md`.
- Find the Unreal project root by locating the nearest `*.uproject` file.
- Run build, test, and file-path-sensitive commands from the Unreal project root unless a task clearly targets another directory.

If multiple `.uproject` files exist, use the one closest to the files being changed. If that is ambiguous, stop and ask which project is in scope.

## Core Rules

- Use superpowers for design and implementation plan whenever possible.
- Leave all changes as unstaged working tree changes.
- Do not create commits unless the user explicitly tells you to commit.
- Do not stage files with `git add` unless the user explicitly asks for staging.
- Do not rewrite history, amend commits, or clean the working tree unless the user explicitly asks for it.
- Do not revert user changes you did not make.

## Unreal Conventions

- Prefer C++ gameplay logic over Blueprint-only logic when implementing systems, unless the user asks for a Blueprint solution.
- Keep authoritative rules and data validation in C++.
- Use Blueprint exposure only where designers or UI need it.
- Prefer reusable components, subsystems, helpers, and data assets over hard-coding logic into one actor.
- Follow existing Unreal naming conventions: `A` for actors, `U` for UObject/component classes, `F` for structs, `E` for enums, `I` for interfaces.
- Keep headers focused and minimize unnecessary includes. Prefer forward declarations where practical.

## File Placement

- Put runtime gameplay code under `Source/<ProjectName>/`.
- Put editor-only code in a separate editor module only when the project already uses one or the task clearly requires it.
- Put config changes under `Config/`.
- Put content assets under `Content/` and avoid renaming or moving large content trees unless required by the task.
- Put design docs or plans under `docs/` when documentation is requested.

## Validation

When validating changes, prefer the narrowest useful check first:
- Build the affected Unreal target or module when feasible.
- Run automated tests that cover the changed code when they exist.
- If full validation is too expensive or unavailable, state exactly what was not verified.

## Communication

- State assumptions clearly when project structure is incomplete.
- When referencing files, use paths relative to the Unreal project root or repository root as appropriate.
- If a task would benefit from a new Unreal project or plugin structure that does not exist yet, work within the current repository unless the user asks to scaffold a new project.
