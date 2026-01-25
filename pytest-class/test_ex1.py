import pytest
from ex1 import *

# simple function test
def test_add():
    assert add(1, 2) == 3
    assert add(-1, 27) == 26

   
# random function test
import numpy as np
def test_add_random():
    a = np.random.randint(-100, 100)
    b = np.random.randint(-100, 100)
    assert add(a,b) == a+b

# test that an exception is raised correctly
def test_divide_by_zero():
    with pytest.raises(ValueError, match="Cannot divide by zero"):
        divide(10, 0)

# using a decorator for testing multiple inputs
@pytest.mark.parametrize("a, b, expected", [
    (10, 2, 5), 
    (20, 5, 4),
    (42, 6, 7),
])
def test_divide_parametrized(a, b, expected):
    assert divide(a, b) == expected
