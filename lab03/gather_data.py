#!/usr/bin/env python3

import subprocess
import re

def run_test(n, threads):
    result = subprocess.run(['./parray', '100', '100', '0', '0', f'{threads}', '3'], input=f'{n}\n{n}\n', capture_output=True, text=True)
    
    pat = re.compile(r'Time:')

    matches = pat.search(result.stdout)
    
    time = result.stdout[matches.span()[0]:-1]

    print(f'{n:>4}:{threads:>4}  {time}')

for t in range(3, 11):
    for n in range(6, 13):
        run_test(pow(2, n), pow(2, t))

