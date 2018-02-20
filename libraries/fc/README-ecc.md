ECC Support
===========

include/fc/crypto/elliptic.hpp defines an interface for some cryptographic
wrapper classes handling elliptic curve cryptography.

Three implementations of this interface exist. One is based on OpenSSL, the
others are based on libsecp256k1 (see https://github.com/bitcoin/secp256k1 ).
The implementation to be used is selected at compile time using the
cmake variable "ECC_IMPL". It can take one of three values, openssl or
secp256k1 or mixed .
The default is "openssl". The alternatives can be configured when invoking
cmake, for example

cmake -D ECC_IMPL=secp256k1 .

If secp256k1 or mixed is chosen, the secp256k1 library and its include file
must already be installed in the appropriate library / include directories on
your system.


Testing
-------

Type "make ecc_test" to build the ecc_test executable from
tests/crypto/ecc_test.cpp with the currently configured ECC implementation.

ecc_test expects two arguments:

ecc_test <pass> <interop-file>

<pass> is a somewhat arbitrary password used for testing.

<interop-file> is a data file containing intermediate test results.
If the file does not exist, it will be created and intermediate results from
the current ECC backend are written to it.
If the file does exist, intermediate results from the current ECC backend
are compared with the file contents.

For a full round of interoperability testing, you can use the script
tests/crypto/ecc-interop.sh .
None of the test runs should produce any output.

