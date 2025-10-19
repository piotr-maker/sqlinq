#!/usr/bin/env python3
import re
import os
import sys
import argparse
from pathlib import Path
import tree_sitter_cpp as tscpp
from tree_sitter import Language, Parser

# --- Setup parser ---
CPP_LANGUAGE = Language(tscpp.language())
parser = Parser(CPP_LANGUAGE)


# --- Helpers ---

def strip_include_guard(source: str) -> str:
    source = re.sub(r"#ifndef\s+\w+\s+#define\s+\w+\s+", "", source, count=1, flags=re.DOTALL)
    source = re.sub(r"#endif\s*(//.*)?", "", source)
    return source


def node_text(node, source_code: bytes) -> str:
    """Return the source text corresponding to a Tree-sitter node."""
    return source_code[node.start_byte:node.end_byte].decode("utf-8")


def extract_table_name(attribute_node_text: str, struct_name: str) -> str:
    """Extract the table name from [[table("...")]], or fall back to the struct name."""
    match = re.search(r'table\("([^"]+)"\)', attribute_node_text, re.IGNORECASE)
    if match:
        return match.group(1)
    return struct_name


def parse_attributes(attr_node, source_code: bytes):
    """Convert [[name("value"), flag]] into a dict: {'name': 'value', 'flag': ''}."""
    text = source_code[attr_node.start_byte:attr_node.end_byte].decode("utf8")
    inner = text.strip()[2:-2].strip()  # remove [[ ]]
    attributes = {}
    for part in inner.split(","):
        part = part.strip()
        if "(" in part and part.endswith(")"):
            key, val = part.split("(", 1)
            val = val[:-1]  # strip ')'
            val = val.strip('"')
            attributes[key.strip()] = val
        else:
            attributes[part] = ""
    return attributes


def validate_attributes(attrs: dict, field_name: str):
    """Validate attributes for a field. Raise ValueError if invalid"""
    allowed_attributes = {
        "primary_key": {"required": False},
        "autoincrement": {"required": False},
        "foreign_key": {"required": True},
        "name": {"required": True},
        "unique": {"required": False},
        "default": {"required": True},
    }
    for key, val in attrs.items():
        if key not in allowed_attributes:
            raise ValueError(f"Unknown attribute '{key}' on field '{field}'")
        requires_value = allowed_attributes[key]['required']
        if requires_value and not val:
            raise ValueError(f"Attribute '{key}' on field '{field_name}' requires a value")
        if not requires_value and val:
            raise ValueError(f"Attribute '{key}' on field '{field_name}' must not have a value")

def cpp_type_to_hcl(cpp_type: str, attrs: dict, backend: str) -> tuple[str, bool]:
    """Map a C++ type into an Atlas HCL type."""
    mapping = {
        "bool": {
            "mysql": "boolean",
            "sqlite": "integer"
        },
        "char": {
            "mysql": "tinyint",
            "sqlite": "text"
        },
        "int8_t": {
            "mysql": "tinyint",
            "sqlite": "text"
        },
        "short": {
            "mysql": "smallint",
            "sqlite": "integer"
        },
        "int16_t": {
            "mysql": "smallint",
            "sqlite": "integer"
        },
        "int": {
            "mysql": "int",
            "sqlite": "integer"
        },
        "int32_t": {
            "mysql": "int",
            "sqlite": "integer"
        },
        "long long": {
            "mysql": "bigint",
            "sqlite": "integer"
        },
        "int64_t": {
            "mysql": "bigint",
            "sqlite": "integer"
        },
        "float": {
            "mysql": "float",
            "sqlite": "real"
        },
        "double": {
            "mysql": "double",
            "sqlite": "integer"
        },
        "std::string": {
            "mysql": "varchar(255)",
            "sqlite": "text"
        },
        "sqlinq::Blob": {
            "mysql": "blob",
            "sqlite": "blob"
        },
        "sqlinq::Date": {
            "mysql": "date",
            "sqlite": "text"
        },
        "sqlinq::Time": {
            "mysql": "time",
            "sqlite": "text"
        },
        "sqlinq::Datetime": {
            "mysql": "datetime",
            "sqlite": "text"
        },
        "sqlinq::Timestamp": {
            "mysql": "timestamp",
            "sqlite": "integer"
        },
    }

    optional_match = re.match(r"std::optional<\s*([^>]+)\s*>", cpp_type)
    if optional_match:
        inner_type = optional_match.group(1).strip()
        type_map = mapping.get(inner_type, {})
        hcl_type = type_map.get(backend, "")
        return (hcl_type, True)
    type_map = mapping.get(cpp_type, {})
    hcl_type = type_map.get(backend, "")
    return (hcl_type, False)


# --- Parsing ---

def parse_structs(source_code: bytes):
    """Parse C++ structs and extract table/field metadata."""
    tree = parser.parse(source_code)
    root_node = tree.root_node
    structs = []

    for struct_node in [n for n in root_node.children if n.type == "struct_specifier"]:
        struct_name_node = struct_node.child_by_field_name("name")
        struct_name = node_text(struct_name_node, source_code)

        # Table attribute
        table_attr_node = next(
            (c for c in struct_node.children if c.type == "attribute_declaration"), None
        )
        table_name = extract_table_name(
            node_text(table_attr_node, source_code) if table_attr_node else "",
            struct_name,
        )

        # Fields
        field_list_node = struct_node.child_by_field_name("body")
        fields = []
        if field_list_node:
            for field_node in [
                c for c in field_list_node.children if c.type == "field_declaration"
            ]:
                type_node = field_node.child_by_field_name("type")
                decl_node = field_node.child_by_field_name("declarator")
                attr_node = next(
                    (c for c in field_node.children if c.type == "attribute_declaration"),
                    None,
                )

                field_name = node_text(decl_node, source_code)
                field_type = node_text(type_node, source_code)
                field_attrs = (
                    parse_attributes(attr_node, source_code) if attr_node else {}
                )
                validate_attributes(field_attrs, field_name)
                field = {
                    "column": field_name,
                    "name": field_name,
                    "type": field_type,
                    "attrs": field_attrs
                }
                if "name" in field["attrs"]:
                    alias = field["attrs"].pop("name")
                    if alias:
                        field["column"] = alias
                fields.append(field)
        structs.append({"name": struct_name, "table": table_name, "fields": fields})
    return structs


# --- Code generation ---

def generate_hpp(structs, out_path: Path, include_dirs):
    """Generate a C++ schema header with Table<T>::meta()."""
    lines = [
        "#pragma once",
        "#include <sqlinq/column.hpp>",
        "#include <sqlinq/table.hpp>",
        ""
    ]
    for inc in include_dirs:
        lines.append(f'#include "{inc}"')   # optionally convert to relative path
    
    lines += [
        "",
        "namespace sqlinq {",
        "",
    ]
    for struct in structs:
        lines.append(f"template <> struct Table<{struct['name']}> {{")
        for idx, field in enumerate(struct["fields"]):
            lines.append(f"  SQLINQ_COLUMN({idx}, {struct['name']}, {field['name']})")
        lines.append("")
        lines.append("  static consteval auto meta() {")
        lines.append("    using namespace sqlinq;")
        lines.append(f"    return make_table<{struct['name']}>(")
        lines.append(f"      \"{struct['table']}\",")

        for i, field in enumerate(struct["fields"]):
            comma = "," if i < len(struct["fields"]) - 1 else ""
            attr_list = "".join(f".{a}()" for a in field["attrs"])
            lines.append(
                f"      SQLINQ_COLUMN_META({struct['name']}, {field['name']}, \"{field['column']}\"){attr_list}{comma}"
            )

        lines.append("    );")
        lines.append("  }")
        lines.append("};")
        lines.append("")
    lines.append("} // namespace sqlinq")

    with open(out_path, "w") as f:
        f.write("\n".join(lines))


def generate_hcl(structs, out_path: Path, schema_name: str, db: str):
    """Generate an Atlas schema.hcl file."""
    lines = []
    lines.append(f'schema "{schema_name}" {{}}')
    lines.append("")
    for struct in structs:
        pk_columns = []
        fk_defs = []

        lines.append(f'table "{struct["table"]}" {{')
        lines.append(f'  schema = schema.{schema_name}')
        for field in struct["fields"]:
            attrs = field['attrs']
            column = field['column']
            type, nullable = cpp_type_to_hcl(field["type"], attrs, db)
            if not type:
                raise ValueError(f"Type {field['type']} is not supported")
            lines.append(f'  column "{column}" {{')
            if nullable:
                lines.append("    null = true")
            else:
                lines.append("    null = false")
            lines.append(f"    type = {type}")
            lines.append("  }")
            if "primary_key" in attrs:
                pk_columns.append(field['column'])
            if "foreign_key" in attrs:
                ref = attrs['foreign_key']
                fk_defs.append((field['column'], ref))
        if pk_columns:
            cols = ", ".join(f'column.{c}' for c in pk_columns)
            lines.append("  primary_key {")
            lines.append(f"    columns = [{cols}]")
            lines.append("  }")
        for idx, (col, ref) in enumerate(fk_defs, start=1):
            ref_table, ref_col = ref.split(".")
            lines.append(f'  foreign_key "fk_{col}_{idx}" {{')
            lines.append(f"    columns = [column.{col}]")
            lines.append(f"    ref_columns = [table.{ref_table}.column.{ref_col}]")
            lines.append("  }")
        lines.append("}")
        lines.append("")
    with open(out_path, "w") as f:
        f.write("\n".join(lines))


# --- Main entry ---

if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="Generate SQLinq schema files from annotated C++ structs.")
    ap.add_argument("--input", action="append", required=True,
                    help="input header file(s) (use multiple --input args)")
    ap.add_argument("--outdir", required=True,
                    help="output directory for generated files")
    ap.add_argument("--schema-name", required=True,
                    help="database schema name")
    args = ap.parse_args()

    # --- Normalize arguments ---
    input_files = [Path(p) for p in args.input]
    outdir = Path(args.outdir)

    # --- Ensure output directory exists ---
    outdir.mkdir(parents=True, exist_ok=True)

    # --- Parse all structs from C++ sources ---
    all_structs = []
    include_files = []
    for f in input_files:
        source = f.read_text()
        source = strip_include_guard(source)
        structs = parse_structs(source.encode("utf-8"))
        if structs:
            include_files.append(f)
        for s in structs:
            s["origin"] = str(f)
        all_structs.extend(structs)

    if not all_structs:
        print("No structs with [[Table]] found in input files.")
        sys.exit(0)

    # --- Generate HPP schema file ---
    origin_includes = sorted({s["origin"] for s in all_structs})
    generate_hpp(all_structs, outdir / "include" / "table_schema.hpp", include_files)

    # --- Generate per-backend HCL files ---
    generate_hcl(all_structs, outdir / "schema.my.hcl", args.schema_name, "mysql")
    generate_hcl(all_structs, outdir / "schema.lt.hcl", args.schema_name, "sqlite")

    print(f"Generated schema for {len(all_structs)} structs into {outdir}")


