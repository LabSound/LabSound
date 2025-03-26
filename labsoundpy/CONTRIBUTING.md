# Contributing to LabSoundPy

Thank you for your interest in contributing to LabSoundPy! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

Please be respectful and considerate of others when contributing to this project. We aim to foster an inclusive and welcoming community.

## Getting Started

1. Fork the repository on GitHub
2. Clone your fork locally
3. Set up the development environment
4. Create a new branch for your changes
5. Make your changes
6. Run tests to ensure your changes don't break existing functionality
7. Submit a pull request

## Development Environment Setup

```bash
# Clone the repository
git clone https://github.com/LabSound/LabSound.git
cd LabSound/labsoundpy

# Install in development mode
pip install -e .
```

## Coding Standards

### Python Code

- Follow [PEP 8](https://www.python.org/dev/peps/pep-0008/) style guidelines
- Use Google-style docstrings
- Use type hints where appropriate
- Write unit tests for new functionality

### C++ Code

- Follow the existing code style in the LabSound project
- Use C++17 features where appropriate
- Document all public APIs
- Write unit tests for new functionality

## Testing

Before submitting a pull request, make sure all tests pass:

```bash
# Run Python tests
python -m unittest discover -s tests

# Build and run C++ tests
cd build
cmake --build . --target test
```

## Pull Request Process

1. Update the README.md or documentation with details of changes if appropriate
2. Update the examples if you've added or changed functionality
3. Make sure all tests pass
4. The pull request will be reviewed by maintainers
5. Address any feedback from the review
6. Once approved, your changes will be merged

## Adding New Features

When adding new features, please follow these guidelines:

1. Discuss major changes in an issue before implementing
2. Ensure the feature is well-documented
3. Add appropriate tests
4. Add examples demonstrating the feature
5. Update the API documentation

## Reporting Bugs

When reporting bugs, please include:

1. A clear description of the bug
2. Steps to reproduce the bug
3. Expected behavior
4. Actual behavior
5. Environment information (OS, Python version, etc.)

## Feature Requests

Feature requests are welcome! Please include:

1. A clear description of the feature
2. Use cases for the feature
3. Any relevant examples or references

## License

By contributing to LabSoundPy, you agree that your contributions will be licensed under the same license as the project.
