# Roadmap System

This folder contains feature planning and tracking for the project. Claude Code uses this system to organize work and maintain continuity across sessions.

## Folder Structure

```
roadmap/
├── README.md        # You are here
├── backlog/         # Ideas and requests not yet planned
├── planned/         # Detailed plans ready to build
├── in-progress/     # Currently being worked on
└── completed/       # Finished features for reference
```

## Workflow

### 1. Capture Ideas → backlog/
When a new feature is requested, create a file in `backlog/` with basic information. This is the "idea parking lot."

### 2. Plan Features → planned/
When ready to work on a feature, move it to `planned/` and fill in the technical approach, subtasks, and dependencies.

### 3. Build Features → in-progress/
When actively working on a feature, move it to `in-progress/`. Only one or two items should be here at a time.

### 4. Complete Features → completed/
When done, move to `completed/` for reference. These serve as documentation of what was built and decisions made.

## File Naming Convention

Use numbered prefixes to track order:
```
000-initial-setup.md
001-user-authentication.md
002-categories-feature.md
003-dark-mode.md
```

## How Claude Uses This System

1. **New feature request** → Creates file in backlog/
2. **Planning session** → Moves to planned/, fills in details
3. **Starting work** → Moves to in-progress/, updates CLAUDE.md
4. **Completing work** → Moves to completed/, suggests next item
5. **New session** → Reviews in-progress/ and planned/ to understand priorities
