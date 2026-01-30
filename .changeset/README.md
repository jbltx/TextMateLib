# Changesets

This repository uses [changesets](https://github.com/changesets/changesets) for version management and changelog generation.

## Adding a Changeset

When you've made changes that should be released, create a changeset:

```bash
npm run changeset
```

This will:
1. Prompt you to select which packages have changed
2. Ask for the type of change (major, minor, patch)
3. Ask for a summary of the changes
4. Create a changeset file in this directory

## Changeset Types

- **Major**: Breaking changes that require users to update their code
- **Minor**: New features that are backwards compatible
- **Patch**: Bug fixes and improvements that are backwards compatible

## Before Release

When ready to release:

```bash
npm run changeset:version
```

This will:
1. Update package versions
2. Generate/update CHANGELOG.md files
3. Create a git commit (manual commit may be needed)

Then publish:

```bash
npm run changeset:publish
```

## Example

```bash
# Add a changeset for a new feature
npm run changeset

# When selecting packages: select "textmatelib"
# Type: "minor"
# Summary: "Add support for custom WASM paths"

# Later, when releasing:
npm run changeset:version
npm run changeset:publish
```

## More Information

- [Changesets Documentation](https://github.com/changesets/changesets)
- [Monorepo Workflow](https://github.com/changesets/changesets/blob/main/docs/monorepos.md)
