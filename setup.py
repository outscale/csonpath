import os
import platform
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class BuildExt(build_ext):
    """Custom build_ext that forces mingw32 on Windows."""

    def finalize_options(self):
        if platform.system() == "Windows" and self.compiler is None:
            self.compiler = "mingw32"
        super().finalize_options()


# Base sources
sources = ["csonpath_python.c"]
extra_compile_args = ["-O3", "-I./"]

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
    cmdclass={"build_ext": BuildExt},
)
