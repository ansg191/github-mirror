import sys
from pathlib import Path

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
var_name = sys.argv[3]

with input_path.open() as f:
    lines = f.readlines()

output_path.parent.mkdir(parents=True, exist_ok=True)
with output_path.open("w") as f:
    f.write(
        f'''#ifndef QUERIES_{var_name.upper()}_H
#define QUERIES_{var_name.upper()}_H

'''
    )
    f.write(f'const char *{var_name} =\n')
    for line in lines:
        escaped = line.rstrip('\n').replace('\\', '\\\\').replace('"', '\\"')
        f.write(f'    "{escaped}\\n"\n')
    f.write(';\n')
    f.write(
        f'''
#endif // QUERIES_{var_name.upper()}_H
''')
