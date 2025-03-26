# Setting Up PyPI Publishing for LabSoundPy

This document outlines the steps needed to set up automatic publishing to PyPI for the LabSoundPy package.

## Prerequisites

- A PyPI account
- A GitHub repository for the project
- GitHub Actions enabled for the repository

## Steps to Implement

### 1. Package Configuration

- [ ] Update `setup.py` with all necessary metadata and dependencies
- [ ] Create `pyproject.toml` for build system requirements
- [ ] Create `MANIFEST.in` to include all C++ source files and headers
- [ ] Ensure version information is properly managed in the package

### 2. Build System for C++ Extensions

- [ ] Configure CMake to build LabSound and Python bindings
- [ ] Set up cross-platform build process
- [ ] Install and configure cibuildwheel for building wheels
- [ ] Test building on different platforms (Windows, macOS, Linux)

### 3. CI/CD Pipeline

- [ ] Create GitHub Actions workflow for building and testing
- [ ] Set up PyPI authentication using GitHub Secrets
- [ ] Configure workflow to trigger on new tags
- [ ] Test the CI/CD pipeline with a test PyPI repository

### 4. GitHub Actions Workflow

Create a `.github/workflows/publish.yml` file with content similar to:

```yaml
name: Build and Publish

on:
  push:
    tags:
      - 'v*'

jobs:
  build_wheels:
    name: Build wheels on ${{ matrix.os }}
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]

    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive

      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.10'

      - name: Install cibuildwheel
        run: python -m pip install cibuildwheel

      - name: Build wheels
        run: python -m cibuildwheel --output-dir wheelhouse
        env:
          CIBW_BUILD: "cp38-* cp39-* cp310-* cp311-*"
          CIBW_SKIP: "*-musllinux*"
          CIBW_BEFORE_BUILD: "pip install cmake nanobind"

      - name: Upload wheels
        uses: actions/upload-artifact@v3
        with:
          name: wheels-${{ matrix.os }}
          path: ./wheelhouse/*.whl

  build_sdist:
    name: Build source distribution
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive

      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.10'

      - name: Install dependencies
        run: python -m pip install build

      - name: Build sdist
        run: python -m build --sdist

      - name: Upload sdist
        uses: actions/upload-artifact@v3
        with:
          name: sdist
          path: dist/*.tar.gz

  publish:
    needs: [build_wheels, build_sdist]
    runs-on: ubuntu-latest
    steps:
      - name: Download artifacts
        uses: actions/download-artifact@v3
        with:
          path: dist

      - name: Prepare distribution files
        run: |
          mkdir -p dist_final
          cp dist/wheels-*/*.whl dist_final/
          cp dist/sdist/*.tar.gz dist_final/

      - name: Publish to PyPI
        uses: pypa/gh-action-pypi-publish@release/v1
        with:
          user: __token__
          password: ${{ secrets.PYPI_API_TOKEN }}
          packages-dir: dist_final/
```

### 5. Challenges Specific to LabSoundPy

- [ ] Ensure all LabSound dependencies are properly built and linked
- [ ] Test cross-platform compatibility
- [ ] Verify binary compatibility across different OS versions
- [ ] Handle platform-specific audio backends

### 6. Documentation and Testing

- [ ] Create comprehensive documentation
- [ ] Set up automated tests that run on the built packages
- [ ] Include example scripts that demonstrate functionality

## Resources

- [PyPI Publishing with GitHub Actions](https://packaging.python.org/guides/publishing-package-distribution-releases-using-github-actions-ci-cd-workflows/)
- [cibuildwheel Documentation](https://cibuildwheel.readthedocs.io/)
- [nanobind Documentation](https://nanobind.readthedocs.io/)
- [Python Packaging User Guide](https://packaging.python.org/)

## Notes

- Consider using Test PyPI first to verify the publishing process
- Implement semantic versioning for the package
- Automate version bumping in the code
- Use git tags to trigger the CI/CD pipeline
