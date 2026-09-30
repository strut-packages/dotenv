include <dotenv>;

function main() -> int : FilesystemError {
    DotenvResult parsed := dotenv_read("fixture.env");
    print(parsed.valid);
    print(parsed.values.length());
    print(parsed.entries.length());
    print(parsed.values["A"]);
    print(parsed.values["B"]);
    print(parsed.values["C"]);
    print(parsed.values["EMPTY"] == "");
    print(parsed.values["D"]);
    print(parsed.values["E"]);
    print(parsed.values["UNICODE"]);

    DotenvResult malformed := dotenv_parse("1BAD=x\nBROKEN\nQ=\"unterminated\n");
    print(malformed.valid);
    print(malformed.diagnostics.length());
    for (diagnostic : malformed.diagnostics) {
        print(diagnostic.code);
        print(diagnostic.line);
        print(diagnostic.column);
    }

    DotenvResult large := dotenv_read("large.env");
    print(large.valid);
    print(large.entries.length());
    return 0;
}
