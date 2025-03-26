from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext
import os
import sys
import subprocess

# This setup.py is designed to build the nanobind-based Python bindings for LabSound

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        try:
            subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError("CMake must be installed to build the extension")
        
        for ext in self.extensions:
            self.build_extension(ext)
    
    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        
        # Required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep
        
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release"
        ]
        
        build_args = ["--config", "Release"]
        
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
            
        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args, 
            cwd=self.build_temp
        )
        subprocess.check_call(
            ["cmake", "--build", "."] + build_args, 
            cwd=self.build_temp
        )

setup(
    name="labsound",
    version="0.1.0",
    author="LabSound Contributors",
    author_email="your.email@example.com",
    description="Python bindings for LabSound audio library",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/LabSound/LabSound",
    packages=find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.7",
    ext_modules=[CMakeExtension("labsound._core")],
    cmdclass={"build_ext": CMakeBuild},
    install_requires=[
        "numpy>=1.19.0",
    ],
    extras_require={
        "dev": [
            "pytest>=6.0",
            "matplotlib>=3.3.0",
        ],
    },
)
