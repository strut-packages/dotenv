# dotenv

Official deterministic dotenv parser for Strut.

## API

```strut
function dotenv_parse(string source) -> DotenvResult;
function dotenv_parse_bytes(bytes source) -> DotenvResult;
function dotenv_read(string path) -> DotenvResult : FilesystemError;
```

`DotenvResult` contains `valid`, a last-value-wins `values` map, source-ordered `entries`, and structured `diagnostics` with stable codes and 1-based byte positions.

## Usage

```strut
include <dotenv>;

function main() -> int : FilesystemError {
    DotenvResult result := dotenv_read(".env");
    if (!result.valid) {
        return 1;
    }
    print(result.values["HOST"]);
    return 0;
}
```

## v0.1 contract

- Keys match `[A-Za-z_][A-Za-z0-9_]*`.
- Spaces and tabs around the key and `=` are ignored.
- Empty, unquoted, single-quoted, and double-quoted values are supported.
- Double quotes recognize `\\`, `\"`, `\n`, `\r`, and `\t`.
- Single-quoted contents and unquoted backslashes are literal.
- Full-line comments and whitespace-preceded trailing comments are supported.
- Optional `export` prefixes are accepted.
- LF, CRLF, bare CR, final lines without terminators, and arbitrary non-NUL value bytes are handled deterministically.
- Duplicate keys use the last value in `values`; `entries` preserves every assignment.
- Malformed lines produce diagnostics and are omitted from values.

Interpolation is intentionally not supported. Input is never evaluated as shell code, and this package does not mutate the process environment.

## Test

```sh
python3 tests/run.py --strut /path/to/strut
```
