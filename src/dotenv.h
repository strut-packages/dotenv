include <filesystem>;
include <map>;

struct DotenvDiagnostic {
    string code;
    string message;
    int_64 line;
    int_64 column;
}

struct DotenvEntry {
    string key;
    string value;
    int_64 line;
}

struct DotenvResult {
    bool valid;
    map<string,string> values;
    DotenvEntry[] entries;
    DotenvDiagnostic[] diagnostics;
}

function dotenv_internal_space(uint_8 value) -> bool {
    return value == 32 || value == 9;
}

function dotenv_internal_key_start(uint_8 value) -> bool {
    return (value >= 65 && value <= 90) || (value >= 97 && value <= 122) || value == 95;
}

function dotenv_internal_key_byte(uint_8 value) -> bool {
    return dotenv_internal_key_start(value) || (value >= 48 && value <= 57);
}

function dotenv_internal_export(bytes source, int_64 begin, int_64 end) -> bool {
    if (end - begin < 7) {
        return false;
    }
    return source[begin] == 101
    && source[begin + 1] == 120
    && source[begin + 2] == 112
    && source[begin + 3] == 111
    && source[begin + 4] == 114
    && source[begin + 5] == 116
    && dotenv_internal_space(source[begin + 6]);
}

function dotenv_internal_diagnostic(
string code,
string message,
int_64 line,
int_64 column
) -> DotenvDiagnostic {
    return DotenvDiagnostic {
        code: code,
        message: message,
        line: line,
        column: column
    };
}

function dotenv_internal_parse_bytes(bytes source) -> DotenvResult {
    map<string,string> values;
    DotenvEntry[] entries := [];
    DotenvDiagnostic[] diagnostics := [];
    int_64 length := source.length();
    int_64 offset := 0;
    int_64 line := 1;

    while (offset < length) {
        int_64 line_start := offset;
        int_64 line_end := offset;
        while (line_end < length && source[line_end] != 10 && source[line_end] != 13) {
            line_end++;
        }
        if (line_end < length && source[line_end] == 13 && line_end + 1 < length && source[line_end + 1] == 10) {
            offset = line_end + 2;
        } else if (line_end < length) {
            offset = line_end + 1;
        } else {
            offset = length;
        }

        bool has_nul := false;
        int_64 nul_column := 1;
        for (int_64 scan := line_start; scan < line_end; scan++) {
            if (source[scan] == 0 && !has_nul) {
                has_nul = true;
                nul_column = scan - line_start + 1;
            }
        }
        if (has_nul) {
            diagnostics.push(dotenv_internal_diagnostic(
            "nul_byte",
            "NUL byte is not allowed",
            line,
            nul_column
            ));
            line++;
            continue;
        }

        int_64 cursor := line_start;
        while (cursor < line_end && dotenv_internal_space(source[cursor])) {
            cursor++;
        }
        if (cursor == line_end || source[cursor] == 35) {
            line++;
            continue;
        }

        if (dotenv_internal_export(source, cursor, line_end)) {
            cursor = cursor + 6;
            while (cursor < line_end && dotenv_internal_space(source[cursor])) {
                cursor++;
            }
        }

        if (cursor == line_end || !dotenv_internal_key_start(source[cursor])) {
            diagnostics.push(dotenv_internal_diagnostic(
            "invalid_key_start",
            "variable name must start with ASCII letter or underscore",
            line,
            cursor - line_start + 1
            ));
            line++;
            continue;
        }

        int_64 key_start := cursor;
        while (cursor < line_end && dotenv_internal_key_byte(source[cursor])) {
            cursor++;
        }
        int_64 key_end := cursor;
        if (cursor < line_end && !dotenv_internal_space(source[cursor]) && source[cursor] != 61) {
            diagnostics.push(dotenv_internal_diagnostic(
            "invalid_key_character",
            "variable name contains an invalid character",
            line,
            cursor - line_start + 1
            ));
            line++;
            continue;
        }
        while (cursor < line_end && dotenv_internal_space(source[cursor])) {
            cursor++;
        }
        if (cursor == line_end || source[cursor] != 61) {
            diagnostics.push(dotenv_internal_diagnostic(
            "missing_equals",
            "expected '=' after variable name",
            line,
            cursor - line_start + 1
            ));
            line++;
            continue;
        }
        cursor++;
        while (cursor < line_end && dotenv_internal_space(source[cursor])) {
            cursor++;
        }

        string value := "";
        bool value_valid := true;
        bool quoted_value := false;
        if (cursor < line_end && source[cursor] == 39) {
            quoted_value = true;
            int_64 quote_column := cursor - line_start + 1;
            cursor++;
            int_64 value_start := cursor;
            while (cursor < line_end && source[cursor] != 39) {
                cursor++;
            }
            if (cursor == line_end) {
                diagnostics.push(dotenv_internal_diagnostic(
                "unterminated_single_quote",
                "unterminated single-quoted value",
                line,
                quote_column
                ));
                value_valid = false;
            } else {
                value = source.slice(value_start, cursor).to_string();
                cursor++;
            }
        } else if (cursor < line_end && source[cursor] == 34) {
            quoted_value = true;
            int_64 quote_column := cursor - line_start + 1;
            cursor++;
            bytes decoded := bytes(line_end - cursor);
            int_64 used := 0;
            bool closed := false;
            while (cursor < line_end) {
                uint_8 current := source[cursor];
                if (current == 34) {
                    closed = true;
                    cursor++;
                    break;
                }
                if (current == 92) {
                    int_64 escape_column := cursor - line_start + 1;
                    cursor++;
                    if (cursor == line_end) {
                        diagnostics.push(dotenv_internal_diagnostic(
                        "invalid_escape",
                        "invalid escape in double-quoted value",
                        line,
                        escape_column
                        ));
                        value_valid = false;
                        break;
                    }
                    uint_8 escaped := source[cursor];
                    if (escaped == 110) {
                        decoded[used] = 10;
                    } else if (escaped == 114) {
                        decoded[used] = 13;
                    } else if (escaped == 116) {
                        decoded[used] = 9;
                    } else if (escaped == 34) {
                        decoded[used] = 34;
                    } else if (escaped == 92) {
                        decoded[used] = 92;
                    } else {
                        diagnostics.push(dotenv_internal_diagnostic(
                        "invalid_escape",
                        "invalid escape in double-quoted value",
                        line,
                        escape_column
                        ));
                        value_valid = false;
                        break;
                    }
                    used++;
                    cursor++;
                    continue;
                }
                decoded[used] = current;
                used++;
                cursor++;
            }
            if (value_valid && !closed) {
                diagnostics.push(dotenv_internal_diagnostic(
                "unterminated_double_quote",
                "unterminated double-quoted value",
                line,
                quote_column
                ));
                value_valid = false;
            }
            if (value_valid) {
                value = decoded.slice(0, used).to_string();
            }
        } else {
            int_64 value_start := cursor;
            int_64 value_end := line_end;
            while (cursor < line_end) {
                if (source[cursor] == 35 && (cursor == value_start || dotenv_internal_space(source[cursor - 1]))) {
                    value_end = cursor;
                    break;
                }
                cursor++;
            }
            while (value_end > value_start && dotenv_internal_space(source[value_end - 1])) {
                value_end--;
            }
            value = source.slice(value_start, value_end).to_string();
        }

        if (value_valid && cursor < line_end) {
            bool had_space := false;
            while (cursor < line_end && dotenv_internal_space(source[cursor])) {
                had_space = true;
                cursor++;
            }
            if (cursor < line_end && (source[cursor] != 35 || (quoted_value && !had_space))) {
                diagnostics.push(dotenv_internal_diagnostic(
                "trailing_content",
                "unexpected content after quoted value",
                line,
                cursor - line_start + 1
                ));
                value_valid = false;
            }
        }

        if (value_valid) {
            string key := source.slice(key_start, key_end).to_string();
            values.insert(key, value);
            entries.push(DotenvEntry { key: key, value: value, line: line });
        }
        line++;
    }

    return DotenvResult {
        valid: diagnostics.empty(),
        values: values,
        entries: entries,
        diagnostics: diagnostics
    };
}

function dotenv_internal_parse(string source) -> DotenvResult {
    return dotenv_internal_parse_bytes(bytes.from_string(source));
}

function dotenv_internal_read(string path) -> DotenvResult : FilesystemError {
    return dotenv_internal_parse_bytes(read_bytes(path));
}

struct DotenvFacade {
    function parse(string source) -> DotenvResult;
    function parse_bytes(bytes source) -> DotenvResult;
    function read(string path) -> DotenvResult : FilesystemError;
}

function DotenvFacade::parse(string source) -> DotenvResult { return dotenv_internal_parse(source); }
function DotenvFacade::parse_bytes(bytes source) -> DotenvResult { return dotenv_internal_parse_bytes(source); }
function DotenvFacade::read(string path) -> DotenvResult : FilesystemError { return dotenv_internal_read(path); }

function dotenv_internal_facade() -> DotenvFacade { return DotenvFacade {}; }

DotenvFacade dotenv := dotenv_internal_facade();
