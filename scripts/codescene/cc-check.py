"""Report functions exceeding CodeScene's limits: cyclomatic complexity (Complex Method),
the number of arguments (Excess Number of Function Arguments), the logical operators
in a branch's condition (Complex Conditional), the nested blocks (Bumpy Road), the function's
lines of code (Large Method) and the file's ones (Lines of Code in a Single File).

Usage: python scripts/codescene/cc-check.py [-C 8] [-A 4] [-L 1] [-B 1] [-M 69] [-F 999]
       [--changed] [<files>...]
  --changed   also check the .c/.cpp/.h files changed vs HEAD (staged, unstaged and untracked)
              and the braces of their changed if's bodies (check-if-braces.py HEAD)
Run it from the repository's root: the changed files' paths are relative to it.

lizard loses track of the functions after file-scope static_assert()s,
so they are blanked out (keeping the line numbers) before the analysis.

lizard doesn't count the switch's "default:" labels and the "continue"s, CodeScene does
(fort_conf_rule_filter_check_equal(): lizard 9, CodeScene 10;
fort_conf_rules_valid(): lizard 9, CodeScene 10), so they are added.
CodeScene reports a Complex Method from cc = 9 (fort_conf_io_valid()).

A Complex Conditional is an if/while condition with 2+ "&&"/"||"
(fort_conf_rules_valid(): 2, fort_conf_io_valid(): 3 by CodeScene).

A bump is a body (an if/else branch, a loop's or switch's body) of a function's top-level
statement, which contains another if/loop/switch; CodeScene reports a Bumpy Road from 2 bumps
(fortps.c: fort_pstree_handle_opened_proc_locked() and fort_pstree_update_service_proc()
have 2 bumps, the file's other functions have at most 1).

The defaults are CodeScene's C++ ones (code-health-rules.cpp.json of "What Are the Default
Threshold Values for Different Programming Languages?", helpcenter.codescene.com): the warnings'
thresholds are reached by the value itself (cyclomatic_complexity_warning 9, bumpy_road_bumps 2,
complex_conditional_branches 2), so Large Method is function_lines_of_code_warning 70 (reported from
70 lines of code, lizard's nloc: non-blank, non-comment lines, the signature included) and
Lines of Code in a Single File is file_lines_of_code_for_warning 1000 (from 1000, lizard's file nloc).
Not verified whether CodeScene reports exactly 70 or 1000, or only more.

Duplicated Function Blocks: a pair of a file's functions of 10+ lines of code (lizard's nloc, the
signature included: CodeScene's default function_duplication_min_lines_of_code_for_check),
whose tokens are similar by 0.96+ (difflib's ratio) with the identifiers and numbers generalized
and which share 0.4+ of their identifiers (Jaccard index; the literals generalized). Fitted on 30
pairs of CodeScene's reports: fortdev.c @5ee96826e (the 8 set* controls), fortprov.c @c85ba5943
(init_boot/persist/reauth_filters, add_callouts/add_filters), fortcout.c @5ee96826e (none) and
logbuffer.cpp @b43f0a485 (write/readEntryApp/ProcNew, readEntryProcKill/FlowClosed). Not reported
e.g.: fort_prov_init_packet_filters/_discard_filters (tokens 0.99, identifiers 0.31: the lists of
different GUIDs), fort_device_control_pending/_allowpending (tokens 0.93), LogBuffer::readEntryTime/
readEntryProcKill (tokens 0.88), fort_callout_install_ale/_packet (6 lines of code).
"""

import argparse
import difflib
import os
import re
import subprocess
import sys

import lizard

STATIC_ASSERT_RE = re.compile(r"^[ \t]*static_assert\s*\(.*?\)\s*;", re.MULTILINE | re.DOTALL)

COMMENT_RE = re.compile(r"//[^\n]*|/\*.*?\*/", re.DOTALL)

STRING_RE = re.compile(r"\"(?:\\.|[^\"\\\n])*\"|'(?:\\.|[^'\\\n])*'")

PREPROCESSOR_RE = re.compile(r"^[ \t]*#.*$", re.MULTILINE)

CC_EXTRA_RE = re.compile(r"\bdefault\s*:(?!:)|\bcontinue\s*;")

BRANCH_RE = re.compile(r"(?<![#\w])(?:if|while)\s*\(")

LOGICAL_RE = re.compile(r"&&|\|\|")

TOKEN_RE = re.compile(r"[A-Za-z_]\w*|\S")

SOURCE_EXTS = (".c", ".cpp", ".h")

DUP_TOKEN_RE = re.compile(r"[A-Za-z_]\w*|\d\w*|->|::|&&|\|\||[=!<>]=|\+=|\S")

DUP_KEYWORDS = frozenset("""if else for while do switch case default return break continue goto
sizeof const static inline void int unsigned char struct typedef enum union""".split())

DUP_MIN_NLOC = 10
DUP_MIN_TOKENS_RATIO = 0.96
DUP_MIN_IDS_RATIO = 0.4


def blank_static_asserts(code):
    return STATIC_ASSERT_RE.sub(lambda m: "\n" * m.group(0).count("\n"), code)


def blank_comments_strings(code):
    """Replaces the comments and literals by spaces, keeping the line numbers."""

    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))

    return STRING_RE.sub(blank, COMMENT_RE.sub(blank, code))


def changed_files():
    out = subprocess.run(
        ["git", "status", "--porcelain", "--untracked-files=all"],
        capture_output=True, text=True, check=True).stdout

    files = []
    for line in out.splitlines():
        path = line[3:].split(" -> ")[-1].strip('"')
        if line[:2].strip() != "D" and path.endswith(SOURCE_EXTS):
            files.append(path)
    return files


def func_body(func, lines):
    return "\n".join(lines[func.start_line - 1 : func.end_line])


def func_cc(func, body):
    return func.cyclomatic_complexity + len(CC_EXTRA_RE.findall(body))


def branch_conditions(body):
    """Yields (line offset, condition) of the if/while statements."""
    for m in BRANCH_RE.finditer(body):
        depth = 1
        pos = m.end()
        while pos < len(body) and depth != 0:
            depth += {"(": 1, ")": -1}.get(body[pos], 0)
            pos += 1

        yield body.count("\n", 0, m.start()), body[m.end() : pos - 1]


class StatementParser:
    """Parses the statements' structure of a function's body tokens."""

    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def next(self):
        self.pos += 1
        return self.tokens[self.pos - 1]

    def skip_balanced(self, open_tok, close_tok):
        depth = 0
        while self.peek() is not None:
            tok = self.next()
            if tok == open_tok:
                depth += 1
            elif tok == close_tok:
                depth -= 1
                if depth == 0:
                    return

    def statement(self):
        """Returns (is control statement or contains one, the bodies' "contains one" flags)."""
        tok = self.peek()

        if tok == "{":
            return self.compound(), []

        if tok in ("if", "while", "for", "switch"):
            return True, self.control(tok)

        if tok == "do":
            return True, self.do_while()

        if tok in ("case", "default"):
            while self.peek() not in (":", None):
                self.next()
            self.next()
            return False, []

        self.simple()
        return False, []

    def compound(self):
        self.next()  # {
        has_control = False
        while self.peek() not in ("}", None):
            has_control |= self.statement()[0]
        if self.peek() == "}":
            self.next()
        return has_control

    def control(self, keyword):
        self.next()
        self.skip_balanced("(", ")")
        bodies = [self.statement()[0]]

        while keyword == "if" and self.peek() == "else":
            self.next()
            if self.peek() == "if":  # else if: the chain's next branch
                self.next()
                self.skip_balanced("(", ")")
            bodies.append(self.statement()[0])

        return bodies

    def do_while(self):
        self.next()
        bodies = [self.statement()[0]]
        if self.peek() == "while":
            self.next()
            self.skip_balanced("(", ")")
        if self.peek() == ";":
            self.next()
        return bodies

    def simple(self):
        """Skips an expression or declaration statement."""
        while self.peek() not in (";", "}", None):
            tok = self.peek()
            if tok == "(":
                self.skip_balanced("(", ")")
            elif tok == "{":
                self.skip_balanced("{", "}")
            else:
                self.next()
        if self.peek() == ";":
            self.next()


def func_bumps(body):
    tokens = TOKEN_RE.findall(PREPROCESSOR_RE.sub("", body))
    if "{" not in tokens:
        return 0

    parser = StatementParser(tokens[tokens.index("{") + 1 :])

    bumps = 0
    while parser.peek() not in ("}", None):
        bumps += sum(parser.statement()[1])
    return bumps


def check_func(path, func, lines, limits):
    body = func_body(func, lines)
    where = f"{path}:{func.start_line}: {func.name}"

    cc = func_cc(func, body)
    if cc > limits.cc:
        yield f"{where}: cc = {cc}"

    if func.parameter_count > limits.args:
        yield f"{where}: arguments = {func.parameter_count}"

    for line_off, cond in branch_conditions(body):
        ops = len(LOGICAL_RE.findall(cond))
        if ops > limits.logical:
            yield f"{path}:{func.start_line + line_off}: {func.name}: conditional's && and || = {ops}"

    bumps = func_bumps(body)
    if bumps > limits.bumps:
        yield f"{where}: bumps = {bumps}"

    if func.nloc > limits.method_lines:
        yield f"{where}: lines of code = {func.nloc}"


def generalize_literals(code):
    """Removes the comments and replaces the literals by "S", keeping the line numbers."""

    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))

    def literal(m):
        return "S" + " " * (len(m.group(0)) - 1)

    return STRING_RE.sub(literal, COMMENT_RE.sub(blank, code))


def is_identifier(tok):
    return tok[0].isalpha() or tok[0] == "_"


def dup_signature(func, lines, literal_lines):
    """Returns (generalized tokens, identifiers) of the function's body from its "{"."""
    start = func_body(func, lines).find("{")
    if start < 0:
        return None

    if func.nloc < DUP_MIN_NLOC:
        return None

    body = func_body(func, literal_lines)[start:]

    raw_tokens = DUP_TOKEN_RE.findall(body)

    tokens = []
    for tok in raw_tokens:
        if tok[0].isdigit():
            tok = "N"
        elif is_identifier(tok) and tok not in DUP_KEYWORDS:
            tok = "I"
        tokens.append(tok)

    return tokens, {tok for tok in raw_tokens if is_identifier(tok)}


def check_duplicates(path, funcs, lines, literal_lines):
    sigs = []
    for func in funcs:
        sig = dup_signature(func, lines, literal_lines)
        if sig:
            sigs.append((func, sig))

    for i, (func, (tokens, ids)) in enumerate(sigs):
        for other, (other_tokens, other_ids) in sigs[i + 1 :]:
            ids_ratio = len(ids & other_ids) / len(ids | other_ids)
            if ids_ratio < DUP_MIN_IDS_RATIO:
                continue

            tokens_ratio = difflib.SequenceMatcher(
                    None, tokens, other_tokens, autojunk=False).ratio()
            if tokens_ratio >= DUP_MIN_TOKENS_RATIO:
                yield (f"{path}:{func.start_line}: {func.name}: duplicates {other.name}"
                       f" (line {other.start_line}; tokens {tokens_ratio:.2f},"
                       f" identifiers {ids_ratio:.2f})")


def check_file(path, limits):
    with open(path, encoding="utf-8", errors="replace") as f:
        code = blank_static_asserts(f.read())

    info = lizard.analyze_file.analyze_source_code(path, code)

    lines = blank_comments_strings(code).splitlines()
    literal_lines = generalize_literals(code).splitlines()

    count = 0
    if info.nloc > limits.file_lines:
        print(f"{path}: file lines of code = {info.nloc}")
        count += 1

    for func in info.function_list:
        for issue in check_func(path, func, lines, limits):
            print(issue)
            count += 1

    for issue in check_duplicates(path, info.function_list, lines, literal_lines):
        print(issue)
        count += 1
    return count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-C", dest="cc", type=int, default=8, help="max allowed cc")
    parser.add_argument(
            "-A", dest="args", type=int, default=4, help="max allowed arguments count")
    parser.add_argument("-L", dest="logical", type=int, default=1,
            help="max allowed && and || in an if/while condition")
    parser.add_argument("-B", dest="bumps", type=int, default=1, help="max allowed bumps")
    parser.add_argument("-M", dest="method_lines", type=int, default=69,
            help="max allowed lines of code of a function")
    parser.add_argument("-F", dest="file_lines", type=int, default=999,
            help="max allowed lines of code of a file")
    parser.add_argument("--changed", action="store_true")
    parser.add_argument("files", nargs="*")
    limits = parser.parse_args()

    files = limits.files + (changed_files() if limits.changed else [])

    count = sum(check_file(path, limits) for path in files)

    # The convention: an `if` returning a computed expression has braces
    if limits.changed:
        script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "check-if-braces.py")
        r = subprocess.run([sys.executable, script, "HEAD"], capture_output=True, text=True,
                           encoding="utf-8")
        lines = r.stdout.splitlines()
        braces = int(lines[-1].split(":")[1]) if lines else 0
        for line in lines[:-1]:
            print(f"{line}: an if returning a computed expression without braces")
        count += braces

    print(f"{count} issue(s) (cc > {limits.cc}, arguments > {limits.args},"
          f" conditional's && and || > {limits.logical}, bumps > {limits.bumps},"
          f" function's lines > {limits.method_lines}, file's lines > {limits.file_lines},"
          f" duplicated functions)"
          f" in {len(files)} file(s)")
    return 1 if count else 0


if __name__ == "__main__":
    sys.exit(main())
