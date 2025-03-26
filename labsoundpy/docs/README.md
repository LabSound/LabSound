# LabSoundPy Documentation

This directory contains documentation for the LabSound Python bindings.

## Documentation Structure

- **API Reference**: Detailed documentation for all classes, methods, and properties
- **Tutorials**: Step-by-step guides for common tasks
- **Examples**: Annotated example scripts demonstrating various features
- **Migration Guide**: Guide for Web Audio API developers moving to Python

## Building the Documentation

The documentation is built using Sphinx. To build the documentation:

```bash
# Install Sphinx and required extensions
pip install sphinx sphinx-rtd-theme

# Build the HTML documentation
cd docs
make html
```

The built documentation will be available in the `_build/html` directory.

## Documentation Guidelines

When contributing to the documentation, follow these guidelines:

1. Use clear, concise language
2. Include code examples for all features
3. Document all parameters, return values, and exceptions
4. Provide links to related documentation
5. Include diagrams where appropriate

## API Documentation

The API documentation is generated from docstrings in the Python code. When writing docstrings, follow these guidelines:

1. Use Google-style docstrings
2. Document all parameters, return values, and exceptions
3. Include examples where appropriate
4. Document any limitations or edge cases

Example:

```python
def my_function(param1, param2=None):
    """
    Brief description of the function.
    
    Longer description explaining the function's behavior.
    
    Args:
        param1: Description of param1
        param2: Description of param2. Defaults to None.
        
    Returns:
        Description of the return value
        
    Raises:
        ValueError: If param1 is invalid
        
    Example:
        >>> my_function(42)
        'Result: 42'
    """
    # Function implementation
```

## Documentation TODOs

- [ ] Complete API reference for all classes
- [ ] Add tutorials for common tasks
- [ ] Add diagrams for audio processing concepts
- [ ] Create a migration guide for Web Audio API developers
