#!/bin/bash

set -xe

apt install -y doxygen

# blockchain

git clone https://github.com/GolosChain/golos

cd golos/

doxygen

cd ..

mkdir data
mkdir data/doc

cp -r golos/documentation/doxygen/html/* data/doc
cd data/doc
ln -s annotated.html blockchain.html
cd ../..


# client

git clone -b gh-pages --single-branch https://github.com/GolosChain/tolstoy gh-pages
mv gh-pages/index.html gh-pages/client.html
rm -rf gh-pages/.git/
rm gh-pages/{CNAME,README.md}
cp -r gh-pages/* data/doc

# testnet & doc

git clone https://bitbucket.org/goloschainru/developers
rm -rf developers/.git/

mv developers/testnet/index.html developers/testnet/testnet.html
cp -r developers/testnet/* data/doc

rm -rf developers/testnet/
cp -r developers/* data

cd data

git init

# non-interactive mode

if [ $1 == "--yes" ]; then
    VAR_DEPLOY="yes"
else 
    echo "---------------------------------------------------------------"
    echo -n "Deploy to developers.golos.io? (type \"yes\" or \"no\"): "
    read VAR_DEPLOY
fi


if [ $VAR_DEPLOY == "yes" ]; then

  git remote add deploy "deploy@developers.golos.io:/www"
  git config user.name "Docs_autodeploy"
  git config user.email "goloschain@gmail.com"

  git add .
  git commit -m "Deploy"
  git push --force deploy master

fi
