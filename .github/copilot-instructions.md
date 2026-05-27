# GitHub Copilot Instructions

This repository contains NUClear — a C++ reactive framework for robot software.

## Project Overview

NUClear is a C++14 message-passing framework built around reactive programming.
Reactors define event-driven callbacks using a domain-specific language (DSL) of template words.
The framework handles threading, scheduling, and data routing automatically.

### Core Components

- **PowerPlant**: Top-level container that manages the system lifecycle
- **Reactor**: Base class for user modules that define reactions
- **DSL Words**: Template parameters that define triggering and data access (`Trigger`, `With`, `Every`, etc.)
- **Threading**: Pool-based scheduler with priority queuing and group synchronisation
- **Extensions**: Plugin system for IO, networking, timers, and tracing

### Build System

- CMake with C++14 target (`cxx_std_14`)
- Catch2 for unit testing (fetched via CMake)
- Platform support: Linux, macOS, Windows

## Code Quality Standards

- **Language Standard**: C++14 (do not use C++17 or later features)
- **Formatting**: clang-format
- **Linting**: clang-tidy (config in `cmake/ClangTidy.cmake`)
- **Testing**: Catch2, run via CTest
- **Language**: American English for all code, comments, and documentation

### Code Style

- **Clean Changes**: When making changes don't leave behind comments describing what was once there; comments should always describe code as it exists
- **API Evolution**: When making changes don't leave behind backwards-compatible interfaces for internal APIs; there should always be a complete clean changeover
- **Property Access**: Assume fields that should exist do exist directly; rely on errors if they don't rather than adding unnecessary null checks

## Documentation Standards

Documentation lives in `docs/` and is built with MkDocs Material.
All documentation files are Markdown.

### Markdown Formatting

- **Formatter**: mdformat (config in `pyproject.toml`)
- **Semantic Line Breaks**: Follow the [Semantic Line Breaks specification (SemBr)](https://sembr.org/)
  - One sentence per line
  - Break after sentences (`.`, `!`, `?`)
  - Break after independent clauses (`,`, `;`, `:`, `—`) when it improves readability
  - Break after dependent clauses for clarity when lines are long
  - **Never break lines based on column count**
  - If a line exceeds ~120 characters, consider simplifying the prose rather than wrapping mid-sentence
  - Example:
    ```markdown
    NUClear is a reactive framework for building modular robot software.
    It uses a domain-specific language embedded in C++ templates to define event-driven callbacks.

    Each module is a Reactor that declares what data it needs,
    and the framework handles threading and scheduling automatically.
    ```

### Writing Style

- **Tone**: Friendly and informative
- **Perspective**: Use second-person ("you" and "your") for user-facing documentation
- **Language**: American English with sentence case for titles
- **Clarity**: Write for non-native English speakers; avoid jargon without explanation
- **Formatting in prose**:
  - Use backticks for: type names, function names, file paths, DSL words, template parameters
  - Use sentence case for headings (capitalize only the first word and proper nouns)
  - Avoid abbreviations when possible

### Documentation Structure (Diátaxis)

The docs follow the [Diátaxis framework](https://diataxis.fr/):

| Section        | Purpose                              | Path               |
| -------------- | ------------------------------------ | ------------------ |
| Getting started | Tutorials for new users             | `getting-started/` |
| Concepts       | Explanation of architecture          | `concepts/`        |
| How-to guides  | Task-oriented recipes                | `how-to/`          |
| Reference      | Complete DSL/API reference           | `reference/`       |
| Explanation    | Deep-dive background material        | `explanation/`     |
| Contributing   | Developer setup and style guide      | `contributing/`    |

### Code Examples in Documentation

- All C++ examples should compile conceptually (correct syntax, proper includes)
- Use `#include "nuclear"` as the canonical include
- Examples should be self-contained where possible
- Verify API usage against source code — do not guess signatures or return types

### Documentation Tooling

```bash
# Install docs dependencies
uv sync --group docs

# Preview documentation
uv run mkdocs serve

# Format all markdown files
uv sync --group dev
uv run mdformat docs/
```

## Development Workflow

### Building

```bash
mkdir build && cd build
cmake .. -GNinja
ninja
```

### Testing

```bash
cd build
ctest
```

### Documentation Development

```bash
uv sync --group docs --group dev
uv run mkdocs serve        # Live preview at http://127.0.0.1:8000
uv run mdformat docs/      # Format markdown
```
