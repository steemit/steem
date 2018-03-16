#!/bin/sh

#TIME=time

cd "`dirname $0`"/..

echo Building ecc_test with openssl...
(
cmake -D ECC_IMPL=openssl .
make ecc_test
mv ecc_test ecc_test.openssl
) >/dev/null 2>&1

echo Building ecc_test with secp256k1...
(
cmake -D ECC_IMPL=secp256k1 .
make ecc_test
mv ecc_test ecc_test.secp256k1
) >/dev/null 2>&1

echo Building ecc_test with mixed...
(
cmake -D ECC_IMPL=mixed .
make ecc_test
mv ecc_test ecc_test.mixed
) >/dev/null 2>&1

run () {
    echo "Running ecc_test.$1 test ecc.interop.$2 ..."
    $TIME "./ecc_test.$1" test "ecc.interop.$2"
}

run openssl openssl
run openssl openssl
run secp256k1 secp256k1
run secp256k1 secp256k1
run mixed mixed
run mixed mixed
run openssl secp256k1
run openssl mixed
run secp256k1 openssl
run secp256k1 mixed
run mixed openssl
run mixed secp256k1

echo Done.

rm -f ecc_test.openssl ecc_test.secp256k1 ecc_test.mixed ecc.interop.openssl ecc.interop.secp256k1 ecc.interop.mixed
