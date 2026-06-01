from setuptools import setup, Extension
from Cython.Build import cythonize
import numpy as np

extensions = [
    Extension(
        "myelin_bsde",
        sources=["bsde.pyx", "bsde_impl.cpp", "ass_impl.cpp"],
        language="c++",
        extra_compile_args=["-std=c++20", "-O3", "-ffast-math", "-ftree-vectorize", "-fomit-frame-pointer", "-funroll-loops"],
        include_dirs=[np.get_include(), "."]
    )
]

setup(
    name="myelin_bsde",
    ext_modules=cythonize(extensions),
)
