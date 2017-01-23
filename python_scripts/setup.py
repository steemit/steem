from setuptools import setup

setup( name='steemdebugnode',
       version='0.1',
       description='A wrapper for launching and interacting with a Golos Debug Node',
       url='http://github.com/steemit/steem',
       author='Golos, Inc.',
       author_email='vandeberg@steemit.com',
       license='See LICENSE.md',
       packages=['steemdebugnode'],
       #install_requires=['steemapi'],
       zip_safe=False )