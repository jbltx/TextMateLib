# Changesets

This folder contains changeset files used for version management and changelog generation.

## What are Changesets?

Changesets are markdown files that describe changes to the project. Each changeset includes:
- A **bump type** (major, minor, or patch)
- A **description** of the change

When processed, changesets are used to:
1. Determine the next version number
2. Generate changelog entries with PR/commit links and author attribution
3. Update version numbers across multiple project files

## Creating a Changeset

Create a new `.md` file in this folder (not `README.md`) with the following format:

```markdown
---
textmatelib: minor
---

Your change description here
```

### Bump Types

- `major` - Breaking changes (e.g., 1.2.3 → 2.0.0)
- `minor` - New features, backward compatible (e.g., 1.2.3 → 1.3.0)
- `patch` - Bug fixes, backward compatible (e.g., 1.2.3 → 1.2.4)

## How It Works

The [bump-versions action](.github/actions/bump-versions/action.yml) processes changesets automatically:

1. **Parses all changesets** in this folder (except `README.md`)
2. **Determines bump type** - uses the highest priority (major > minor > patch)
3. **Extracts metadata** - finds commit SHA, PR number, and author from git history
4. **Calculates new version** - bumps from current version based on bump type
5. **Updates version files**:
   - [CMakeLists.txt](CMakeLists.txt)
   - [conanfile.py](conanfile.py)
   - [src/js/package.json](src/js/package.json)
   - [src/csharp/TextMateLib.Bindings/TextMateLib.Bindings.csproj](src/csharp/TextMateLib.Bindings/TextMateLib.Bindings.csproj)
   - [playground/package.json](playground/package.json)
6. **Generates changelog** - creates formatted entries with links and attribution
7. **Deletes consumed changesets** - removes processed files after use

## Example

If you add a new feature, create `.changeset/cool-feature.md`:

```markdown
---
textmatelib: minor
---

Added support for custom color schemes
```

When the workflow runs, this will:
- Bump the minor version (e.g., 0.1.0 → 0.2.0)
- Generate a changelog entry like: `- [#123](repo/pull/123) by [@username](github/username) - Added support for custom color schemes`
- Update all version files
- Delete `cool-feature.md`
