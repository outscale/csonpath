import platform
import os
import sys
from setuptools import setup, Extension

# Force MinGW compiler on Windows
if platform.system() == "Windows":
    # Set compiler before importing distutils
    os.environ["CC"] = "gcc"
    # Override distutils to use MinGW
    from distutils.cygwinccompiler import Mingw32CCompiler
    from distutils import sysconfig
    sysconfig.get_config_vars()["CC"] = "gcc"

# Base sources
sources = ["csonpath_python.c"]
extra_compile_args=["-O3", "-I./"]

# On Windows, include tiny_regex.c for regex support
if platform.system() == "Windows":
    sources.append("tiny-regex-c/re.c")
    extra_compile_args.append("-DCSONPATH_TINY_REGEX")
    extra_compile_args.append("-I./tiny-regex-c/")

extension = Extension(
    "csonpath",
    sources=sources,
    extra_compile_args=extra_compile_args,
)

setup(
    ext_modules=[extension],
)
