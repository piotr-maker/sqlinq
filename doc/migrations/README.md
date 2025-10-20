# SQLinq Migrations

SQLinq integrates with [Atlas](https://atlasgo.io) to manage database schema migrations.

When you build your project, the `sqlinq_generate_schema()` command automatically produces:
- `schema.my.hcl` — a full schema derived from your annotated structs.
- `schema.lt.hcl` — a lightweight version (optional).
- `table_schema.hpp` — C++ schema metadata.

These HCL files can be used by Atlas CLI to inspect, diff, and apply migrations.

---

## Workflow

### 1. Generate schema files

Build your project:
```bash
cmake --build -DSQLINQ_SCHEMA_NAME=example .
```
Generated files will appear under:
```bash
generated/include/table_schema.hpp
generated/schema.my.hcl
generated/schema.lt.hcl
```

### 2. Generate migration files
```bash
atlas migrate diff \
  --dir "file://migrations/sqlite" \
  --to "file://build/generated/schema.lt.hcl" \
  --dev-url "sqlite://file?mode=memory"
```

### 3. Apply migrations
```bash
atlas migrate apply \
    --dir "file://migrations/sqlite" \
    --url "sqlite://db.sqlite"
```

### Summary
| Step             | Command               | Description                      |
| ---------------- | --------------------- | -------------------------------- |
| Generate schemas | `cmake --build .`     | Produces HCL & header files      |
| Create migration | `atlas migrate diff`  | Generate migration SQL           |
| Apply migration  | `atlas migrate apply` | Apply SQL to database            |

**Tip**: You can validate generated HCL syntax with (Atlas PRO):
```bash
atlas schema lint \
    --url "file://build/generated/schema.lt.hcl" \
    --dev-url "sqlite://file?mode=memory"
```
