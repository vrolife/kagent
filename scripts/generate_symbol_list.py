import sys
import argparse
import re

parser = argparse.ArgumentParser()
parser.add_argument('-o', type=str, default='-')
parser.add_argument('list', type=str)

args = parser.parse_args()

with open(args.list, 'r') as fp:
    lst = fp.read()

if args.o == '-':
    output = sys.stdout
else:
    output = open(args.o, 'w')

SYM = re.compile(r'\s+U\s+(.*)')

output.write(f"""
#include "kagent/private.h"

static const struct SymbolVersion versions[] USED SECTION("__versions") = 
{{
    {{ 0, "module_layout" }},
""")

for m in SYM.findall(lst):
    output.write(f"""
    {{ 0, "{m}" }},
""")

# strtab
output.write(f"""
}};
""")
if output is not sys.stdout:
    output.close()
