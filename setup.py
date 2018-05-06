from setuptools import setup, find_packages, Extension

pybproto = Extension('pybproto',
                     include_dirs = ['./lib/include'],
                     sources = ['python/src/pybproto.c', './lib/bproto.c'],
                     )

setup (name = 'pybproto',
       author = 'Joe Harrison',
       author_email = 'joe@sigwinch.uk',
       url = "https://github.com/sigwinch28/blinken",
       version = '1.0',
       description = 'Python bindings for bproto used in blinken',
       test_suite = "python.test.pybproto_test",
       ext_modules = [pybproto])
