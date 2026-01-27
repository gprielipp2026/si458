#!/usr/bin/env python3

import numpy as np
import sys, os

if len(sys.argv) != 4:
    print(f'{sys.argv[0]} <matrix path A> <matrix path B> <output matrix path>')
    exit(0)

def read(fn):
    M = []
    with open(fn, 'r') as file:
        for line in file.readlines()[1:]:
            M.append( [float(x.strip()) for x in line.split()]  )
    return np.array(M)

def write(fn, M):
    rows, cols = M.shape
       
    np.savetxt(fn + '.tmp', M, fmt='%d')
    with open(fn, 'w') as file:
        with open(fn + '.tmp', 'r') as tmp:
            file.write(f'{rows} {cols}\n')
            file.write(tmp.read())
    os.remove(fn + '.tmp')


A = read(sys.argv[1])
B = read(sys.argv[2])
C = A @ B

write(sys.argv[3], C)
