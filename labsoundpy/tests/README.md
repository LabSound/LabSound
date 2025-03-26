# LabSoundPy Tests

This directory contains tests for the LabSound Python bindings.

## Running Tests

You can run the tests using Python's unittest framework:

```bash
# Run all tests
python -m unittest discover -s tests

# Run a specific test file
python -m unittest tests/test_basic.py

# Run a specific test case
python -m unittest tests.test_basic.TestAudioContext

# Run a specific test method
python -m unittest tests.test_basic.TestAudioContext.test_create_context
```

## Test Structure

The tests are organized into test cases for each major component of the library:

- `TestAudioContext`: Tests for the AudioContext class
- `TestOscillatorNode`: Tests for the OscillatorNode class
- `TestGainNode`: Tests for the GainNode class
- `TestConnections`: Tests for connecting and disconnecting nodes

## Writing New Tests

When adding new tests, follow these guidelines:

1. Create a new test file for each major component or feature
2. Use descriptive test method names that explain what is being tested
3. Include docstrings for test classes and methods
4. Keep tests focused on a single aspect of functionality
5. Use assertions to verify expected behavior

Example:

```python
import unittest
import labsound as ls

class TestMyFeature(unittest.TestCase):
    """Tests for MyFeature."""
    
    def test_specific_behavior(self):
        """Test that MyFeature behaves as expected in a specific scenario."""
        # Test code here
        self.assertEqual(expected, actual)
```

## Test Dependencies

The tests require:

- Python 3.7+
- LabSoundPy installed (either in development mode or from a package)
- unittest (part of the Python standard library)
