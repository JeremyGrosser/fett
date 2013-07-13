from setuptools import setup, Extension

setup(name='fett',
      version='0.1.0',
      license='MIT',
      scripts=['fett'],
      ext_modules=[
          Extension('fett', ['fett.c']),
      ])
