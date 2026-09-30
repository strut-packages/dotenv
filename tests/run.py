#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "tests" / "package_test.p").read_text()
EXPECTED = (
    "1\n7\n8\none\ntwo # still\ntab\tquote\"slash\\\n1\nlast\nok\ncafé\nfirst\nlast\n"
    "0\n3\ninvalid_key_start\n1\n1\nmissing_equals\n2\n7\n"
    "unterminated_double_quote\n3\n3\n0\ntrailing_content\n1\nfour\n"
    "1\n2\ntwo\n0\ninvalid_escape\nunterminated_single_quote\n0\nnul_byte\n1\n1000\n"
)


def run(command, cwd, env):
    return subprocess.run(command, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--strut", default=os.environ.get("STRUT_BIN", "strut"))
    args = parser.parse_args()
    compiler = str(Path(args.strut).resolve()) if Path(args.strut).exists() else args.strut

    with tempfile.TemporaryDirectory(prefix="strut-dotenv-test-") as temporary:
        root = Path(temporary)
        app = root / "app"
        app.mkdir()
        (app / "main.p").write_text(SOURCE)
        (app / "fixture.env").write_bytes(
            b"A=one\r\nB = 'two # still'\nC=\"tab\\tquote\\\"slash\\\\\"\r\n"
            b"EMPTY=\nD=first\nD=last # comment\nexport E = ok\r\nUNICODE=caf\xc3\xa9\n"
        )
        (app / "large.env").write_text("".join(f"KEY_{index}=value-{index}\n" for index in range(1000)))
        (app / "strut.json").write_text(json.dumps({
            "name": "dotenv-package-test",
            "version": "0.1.0",
            "entry": "main.p",
            "dependencies": {},
        }) + "\n")
        env = os.environ.copy()
        env["STRUT_HOME"] = str(root / "strut-home")

        for command in (
            [compiler, "init"],
            [compiler, "add", str(ROOT)],
            [compiler, "install", "--offline"],
            [compiler, "packages", "--json"],
        ):
            result = run(command, app, env)
            if result.returncode:
                print(result.stdout, end="")
                print(result.stderr, end="", file=os.sys.stderr)
                return result.returncode

        output = app / ("dotenv-test.exe" if os.name == "nt" else "dotenv-test")
        result = run([compiler, "main.p", "-o", str(output)], app, env)
        if result.returncode:
            print(result.stdout, end="")
            print(result.stderr, end="", file=os.sys.stderr)
            return result.returncode
        result = run([str(output)], app, env)
        if result.returncode or result.stdout != EXPECTED:
            print("unexpected result", file=os.sys.stderr)
            print("stdout:", repr(result.stdout), file=os.sys.stderr)
            print("stderr:", repr(result.stderr), file=os.sys.stderr)
            return 1

    print("dotenv package test: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
