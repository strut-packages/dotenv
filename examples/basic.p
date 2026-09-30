include <dotenv>;

function main() -> int {
    DotenvResult result := dotenv.parse("HOST=localhost\nPORT=8080\n");
    if (!result.valid) {
        return 1;
    }
    print(result.values["HOST"]);
    print(result.values["PORT"]);
    return 0;
}
