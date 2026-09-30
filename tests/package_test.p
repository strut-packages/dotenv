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
    print(parsed.entries[4].value);
    print(parsed.entries[5].value);

    DotenvResult malformed := dotenv_parse("1BAD=x\nBROKEN\nQ=\"unterminated\n");
    print(malformed.valid);
    print(malformed.diagnostics.length());
    for (diagnostic : malformed.diagnostics) {
        print(diagnostic.code);
        print(diagnostic.line);
        print(diagnostic.column);
    }

    DotenvResult adjacent_comment := dotenv_parse("D=\"four\"#not-a-comment\n");
    print(adjacent_comment.valid);
    print(adjacent_comment.diagnostics[0].code);
    DotenvResult spaced_comment := dotenv_parse("D=\"four\" # comment\n");
    print(spaced_comment.valid);
    print(spaced_comment.values["D"]);

    DotenvResult bare_cr := dotenv_parse("A=one\rB=two\r");
    print(bare_cr.valid);
    print(bare_cr.entries.length());
    print(bare_cr.values["B"]);
    DotenvResult bad_escape := dotenv_parse("A=\"bad\\q\"\nB='unterminated\n");
    print(bad_escape.valid);
    print(bad_escape.diagnostics[0].code);
    print(bad_escape.diagnostics[1].code);
    bytes nul_input := [65, 61, 0, 10];
    DotenvResult nul := dotenv_parse_bytes(nul_input);
    print(nul.valid);
    print(nul.diagnostics[0].code);

    DotenvResult large := dotenv_read("large.env");
    print(large.valid);
    print(large.entries.length());
    return 0;
}
