import pytest
import subprocess

def test_ex2():
    result = subprocess.run(["./array", "100", "15", "0", "1"],
                            input="matrix.txt ",
                            capture_output=True,
                            text=True
                            )
    expected = """Matrix File Path: 
final
------------
16 16 20 16 
16 17 21 20 
-20 -17 21 17
21 -18 21 20
------------
"""

    assert result.stdout.strip() == expected
    assert result.returncode == 0
