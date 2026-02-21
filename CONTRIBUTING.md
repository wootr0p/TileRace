# Contributing to TileRace

Thanks for your interest in contributing to **TileRace**!

This repository uses a protected-branch workflow:
- Direct pushes to `main` and `dev` are **not allowed**
- Changes must go through Pull Requests (PRs)
- At least **1 approval** is required before merging (applies to everyone, including admins)

## Getting started

1. Fork the repository (external contributors) or create a branch (collaborators).
2. Clone your fork/repo locally.
3. Create a new branch from `dev`.
4. Open a Pull Request targeting `dev`.

## Branching model

- `main`: stable / release branch
- `dev`: integration branch for upcoming changes
- Feature branches: short-lived branches created from `dev`

### Branch naming

Use one of the following patterns:

- `feature/<issue-number>-<short-description>`
- `fix/<issue-number>-<short-description>`
- `chore/<short-description>`
- `docs/<short-description>`

Examples:
- `feature/42-online-multiplayer`
- `fix/58-crash-on-pause-menu`
- `chore/refactor-spawn-system`
- `docs/update-readme`

Use lowercase and `-` (kebab-case) in descriptions.

## Issues first (recommended)

Please create or pick an Issue before starting work, especially for new features.
This helps track progress in the project Kanban board and makes reviews easier.

If you are working on an existing issue, mention it in your PR description.

## Making changes

1. Sync with the latest `dev`:
   ```bash
   git fetch origin
   git checkout dev
   git pull
   ```

2. Create a branch:
   ```bash
   git checkout -b feature/<issue-number>-<short-description>
   ```

3. Commit your work in small, focused commits.

4. Push your branch:
   ```bash
   git push -u origin feature/<issue-number>-<short-description>
   ```

## Opening a Pull Request

Open a PR **into `dev`**.

In the PR description, link the issue and enable auto-closing when appropriate by using one of:
- `Fixes #<issue-number>`
- `Closes #<issue-number>`
- `Resolves #<issue-number>`

### PR checklist

Before requesting review:
- [ ] The change is small and focused (or split into multiple PRs)
- [ ] The game still builds and runs
- [ ] New gameplay changes are tested in-game
- [ ] Screenshots / short videos are attached when they help reviewers
- [ ] Documentation is updated if needed

## Code review

- At least **1 approval** is required to merge.
- Address feedback by pushing additional commits to the same branch.
- Resolve review conversations before merging.

## Merging

After approval, merge the PR into `dev`.

Releases are merged from `dev` to `main` via a PR:
- Open PR: `dev` → `main`
- Get at least 1 approval
- Merge

## Reporting bugs

When filing a bug, include:
- Steps to reproduce
- Expected behavior
- Actual behavior
- Platform (OS/device), and any relevant hardware info
- Logs, screenshots, or short videos if possible

## Security

If you discover a security issue, please do not open a public issue.
Contact the maintainers privately.

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.