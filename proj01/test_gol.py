import pytest

import subprocess, re

@pytest.fixture
def exploderActual():
  with open('debug.txt', 'r') as fh:
    return convert2ints(fh.read().split('\n\n'))

def convert2ints(chunks):
  return [[int(row, 2) for row in chunk.replace(' ', '').strip().split('\n')] for chunk in chunks]

def parse_output_gol(data):
  pat = re.compile(r'-{50}')
  
  chunks = pat.split(data)

  return convert2ints(chunks[1::2])


def test_exploder_10(exploderActual):
  result = subprocess.run(['./pargol', '12', '1', '1', '0'], input='0\ndebug.in ', capture_output=True, text=True)
  print(result.stdout)
  toCheck = parse_output_gol(result.stdout)

  for expected, actual in zip(exploderActual, toCheck):
    assert expected == actual


