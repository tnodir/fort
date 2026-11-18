"""Finds an `if` without braces whose body isn't a simple jump (a return of a computed
expression, a call, an assignment), among the lines added by the branch (git diff
master...HEAD). Simple jumps may stay without braces: `continue;`, `break;`, `goto`,
`return;`, a return of a constant (TRUE/FALSE/true/false/NULL/nullptr/{}/a number/an ALL_CAPS
name, a qualified enum like Foo::Bar) or a plain local variable.

Usage: python scripts/codescene/check-if-braces.py [<git diff's revisions>]
  e.g. HEAD: the uncommitted changes and the untracked files (cc-check.py --changed passes it)."""
import os
import re
import subprocess
import sys

# Only the sources: other files (e.g. the installer's .isl) aren't in UTF-8
SOURCE_PATHS = ['*.c', '*.h', '*.cpp']

HUNK = re.compile(r'@@ -\S+ \+(\d+)(?:,(\d+))? @@')

SIMPLE = re.compile(r'^return\s*(;|\{\};|(TRUE|FALSE|true|false|NULL|nullptr|-?\d+|[A-Z]\w*|'
                    r'[A-Za-z_]\w*::[A-Za-z_]\w*|[a-z_]\w*|"[^"]*")\s*;)$')

IF_STARTS = ('if (', '} else if (', 'else if (')

# A jump or a block, or a continued expression: not a body without braces
NOT_BODY_STARTS = ('continue', 'break', 'goto', '{',
                   '.', '&&', '||', '==', '!=', '?', ':', '+', '-', '<<', '>>', ')', ',', '//',
                   '/*', '#')

# The condition's start is looked for within the last few lines
CONDITION_MAX_LINES = 6


def run_git(args, repo):
    return subprocess.run(['git'] + args, cwd=repo, capture_output=True, text=True,
                          encoding='utf-8').stdout


def diff_added_lines(repo, base):
    """The added lines' numbers by the files' paths"""
    added = {}
    cur = None
    diff = run_git(['diff', '-U0', '--no-color', base, '--'] + SOURCE_PATHS, repo)
    for line in diff.splitlines():
        if line.startswith('+++ b/'):
            cur = line[6:].strip()
            added[cur] = set()
            continue
        m = HUNK.match(line)
        if m and cur:
            st = int(m.group(1))
            n = int(m.group(2) or 1)
            added[cur].update(range(st, st + n))
    return added


def untracked_added_lines(repo):
    """All the lines of the untracked files are added"""
    untracked = run_git(['ls-files', '--others', '--exclude-standard', '--'] + SOURCE_PATHS, repo)
    return {f: range(1, sys.maxsize) for f in untracked.splitlines()}


def added_lines(repo, base):
    added = diff_added_lines(repo, base)
    # A diff against the working tree
    if '..' not in base:
        added.update(untracked_added_lines(repo))
    return added


def strip_comment(s):
    s = re.sub(r'/\*.*?\*/', '', s)
    return re.sub(r'//.*$', '', s).strip()


def is_condition_end(s):
    """A one-line if condition without a brace, or the last line of a multi-line one"""
    if s.startswith(('//', '/*', '*')) or s.endswith('{'):
        return False
    return s.startswith(IF_STARTS[:2]) or s.endswith(')')


def is_body(s):
    return bool(s) and not s.startswith(NOT_BODY_STARTS)


def unbraced_body(lines, i, nums):
    """The body without braces after the line i, if it isn't a simple jump and the lines are
    added"""
    body = lines[i + 1].strip()
    if not (is_condition_end(lines[i].strip()) and is_body(body)):
        return None
    if (i + 1) not in nums and (i + 2) not in nums:
        return None
    return None if SIMPLE.match(strip_comment(body)) else body


def condition_start(lines, i):
    """The line of the `if` whose condition ends at the line i, or -1"""
    for start in range(i, max(i - CONDITION_MAX_LINES, -1), -1):
        if lines[start].strip().startswith(IF_STARTS):
            return start
    return -1


def if_start(lines, i):
    """The condition's start, if its parentheses are balanced at the line i, or -1"""
    start = condition_start(lines, i)
    if start < 0:
        return -1
    cond = ' '.join(l.strip() for l in lines[start:i + 1])
    return start if cond.count('(') == cond.count(')') else -1


def read_lines(path):
    try:
        with open(path, encoding='utf-8') as f:
            return f.read().split('\n')
    except FileNotFoundError:
        return []


def check_file(repo, f, nums):
    """Prints the ifs found, returns their count"""
    lines = read_lines(os.path.join(repo, f))
    found = 0
    for i in range(len(lines) - 1):
        body = unbraced_body(lines, i, nums)
        start = -1 if body is None else if_start(lines, i)
        if start < 0:
            continue
        found += 1
        print('%s:%d: %s  ->  %s' % (f, i + 2, lines[start].strip(), body))
    return found


def main():
    repo = subprocess.run(['git', 'rev-parse', '--show-toplevel'], capture_output=True,
                          text=True, check=True).stdout.strip()
    base = sys.argv[1] if len(sys.argv) > 1 else 'master...HEAD'

    added = added_lines(repo, base)
    found = sum(check_file(repo, f, nums) for f, nums in added.items())
    print('found:', found)


if __name__ == '__main__':
    main()
