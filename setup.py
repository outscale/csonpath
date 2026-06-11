import os
import sys
import sysconfig
import platform
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class BuildExt(build_ext):
    """Custom build_ext that forces mingw32 on Windows."""

    def finalize_options(self):
        if platform.system() == "Windows" and self.compiler is None:
            self.compiler = "mingw32"
        super().finalize_options()

    def get_libraries(self, ext):
        libraries = build_ext.get_libraries(self, ext)
        # distutils/setuptools hardcodes the Python library name as pythonXY
        # for mingw32, but free-threading builds (3.13+) on Windows ship
        # pythonXYt.lib instead of pythonXY.lib.
        # We only "fix" this for 3.14+ because 3.13t is considered
        # experimental and we prefer not to link against it.
        if (
            getattr(self.compiler, "compiler_type", None) == "mingw32"
            and platform.system() == "Windows"
            and (sys.version_info.major, sys.version_info.minor) >= (3, 14)
        ):
            version_nodot = f"{sys.version_info.major}{sys.version_info.minor}"
            expected = f"python{version_nodot}"
            if expected in libraries:
                for prefix in (sys.base_prefix, sysconfig.get_config_var("prefix")):
                    if not prefix:
                        continue
                    libs_dir = os.path.join(prefix, "libs")
                    if os.path.exists(os.path.join(libs_dir, f"{expected}t.lib")):
                        libraries = [
                            f"{expected}t" if lib == expected else lib
                            for lib in libraries
                        ]
                        break
        return libraries


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
