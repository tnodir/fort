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

REPO = subprocess.run(['git', 'rev-parse', '--show-toplevel'], capture_output=True, text=True,
                      check=True).stdout.strip()
BASE = sys.argv[1] if len(sys.argv) > 1 else 'master...HEAD'

diff = subprocess.run(['git', 'diff', '-U0', '--no-color', BASE], cwd=REPO,
                      capture_output=True, text=True, encoding='utf-8').stdout

added = {}
cur = None
for line in diff.splitlines():
    if line.startswith('+++ b/'):
        cur = line[6:].strip()
        added[cur] = set()
        continue
    m = re.match(r'@@ -\S+ \+(\d+)(?:,(\d+))? @@', line)
    if m and cur:
        st = int(m.group(1))
        n = int(m.group(2) or 1)
        added[cur].update(range(st, st + n))

# A diff against the working tree: all the lines of the untracked files are added
if '..' not in BASE:
    untracked = subprocess.run(['git', 'ls-files', '--others', '--exclude-standard'], cwd=REPO,
                               capture_output=True, text=True, encoding='utf-8').stdout
    for f in untracked.splitlines():
        added[f] = range(1, sys.maxsize)

SIMPLE = re.compile(r'^return\s*(;|\{\};|(TRUE|FALSE|true|false|NULL|nullptr|-?\d+|[A-Z]\w*|'
                    r'[A-Za-z_]\w*::[A-Za-z_]\w*|[a-z_]\w*|"[^"]*")\s*;)$')


def strip_comment(s):
    s = re.sub(r'/\*.*?\*/', '', s)
    return re.sub(r'//.*$', '', s).strip()

found = 0
for f, nums in added.items():
    if not f.endswith(('.c', '.h', '.cpp')):
        continue
    try:
        lines = open(os.path.join(REPO, f), encoding='utf-8').read().split('\n')
    except FileNotFoundError:
        continue
    for i, line in enumerate(lines):
        s = line.strip()
        if s.startswith(('//', '/*', '*')):
            continue
        # A one-line if condition without a brace, or the last line of a multi-line one
        if not (s.startswith('if (') or s.startswith('} else if (') or s.endswith(')')):
            continue
        if s.endswith('{') or i + 1 >= len(lines):
            continue
        body = lines[i + 1].strip()
        if body.startswith(('continue', 'break', 'goto', '{')) or not body:
            continue
        # A continued expression, not a body
        if body.startswith(('.', '&&', '||', '==', '!=', '?', ':', '+', '-', '<<', '>>', ')', ',', '//', '/*', '#')):
            continue
        # Only the lines added by the branch
        if (i + 1) not in nums and (i + 2) not in nums:
            continue
        # The condition's start: an `if` within the last few lines
        start = i
        while start > 0 and not lines[start].strip().startswith(('if (', '} else if (', 'else if (')):
            start -= 1
            if i - start > 4:
                break
        if not lines[start].strip().startswith(('if (', '} else if (', 'else if (')):
            continue
        cond = ' '.join(l.strip() for l in lines[start:i + 1])
        if cond.count('(') != cond.count(')'):
            continue
        if SIMPLE.match(strip_comment(body)):
            continue
        found += 1
        print('%s:%d: %s  ->  %s' % (f, i + 2, lines[start].strip(), body))

print('found:', found)
