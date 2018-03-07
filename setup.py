from distutils.core import setup, Extension
import os, subprocess
import sys

os.environ["CC"] = "icc"
os.environ["CXX"] = "icc"

srcfiles = ['fft', 'projector', 'pseudoprojector', 'quadrature', 'radial', 'reader', 'sbt', 'tests', 'utils']
cfiles = [f+'.c' for f in srcfiles]
hfiles = [f+'.h' for f in srcfiles]

setup(name='pawpyseed',
	version='0.0.1',
	description='Parallelized DFT wavefunction analysis utilities',
	author='Kyle Bystrom',
	author_email='kylebystrom@berkeley.edu',
	packages=['pawpyseed'],
	package_data={'pawpyseed': cfiles+hfiles+['Makefile', 'pawpy.so']},
	scripts=['pawpy_example.py']
)

if len(sys.argv) > 1 and sys.argv[1] == 'build':
	currdir = os.getcwd()
	os.chdir(os.path.join(currdir,'build/lib/pawpyseed/'))
	subprocess.call('make projector'.split())
	os.chdir(currdir)

