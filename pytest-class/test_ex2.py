import pytest
import subprocess

def test_shell_output():
    # execute shell command
    result = subprocess.run(["echo", "Hello World!"], capture_output=True,
                            text=True)
    assert result.stdout.strip() == "Hello World!"
    assert result.returncode == 0


