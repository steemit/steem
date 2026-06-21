#!/bin/bash
# Deploy and start regression tests on EC2
# Usage: ./deploy_regression.sh <docker_image_tar>
# Example: ./deploy_regression.sh /tmp/test-steem.tar

set -euo pipefail

IMAGE_TAR="${1:?Usage: $0 <docker_image_tar>}"
IMAGE_NAME="ety001/test-steem:latest"
BASE="/steem-regression"

echo "=== Loading Docker image ==="
docker load -i "$IMAGE_TAR"
echo "Image loaded: $IMAGE_NAME"
docker images "$IMAGE_NAME"

echo ""
echo "=== Creating directory structure ==="
mkdir -p "$BASE"/{test-a/data,test-b/data/blockchain,scripts}

echo ""
echo "=== Downloading fullnode.config.ini ==="
# Config will be provided via docker cp or wget from the repo
# For now, create a minimal config that fullnode.config.ini will replace
if [ ! -f "$BASE/test-a/data/config.ini" ]; then
  echo "Waiting for config.ini to be placed in $BASE/test-a/data/"
  echo "Run: docker cp fullnode.config.ini <container>:/steem-regression/test-a/data/config.ini"
fi

echo ""
echo "=== Starting Test A (full sync from block 1) ==="
if docker ps -a --format '{{.Names}}' | grep -q '^steem-test-a$'; then
  echo "steem-test-a already exists, removing..."
  docker rm -f steem-test-a
fi

docker run -d \
  --name steem-test-a \
  -v "$BASE/test-a/data:/var/steem" \
  -p 18091:8091 \
  -p 18090:8090 \
  -p 12001:2001 \
  --restart unless-stopped \
  "$IMAGE_NAME" \
  /usr/local/steemd/bin/steemd \
    --data-dir=/var/steem \
    --webserver-http-endpoint=0.0.0.0:8091 \
    --webserver-ws-endpoint=0.0.0.0:8090 \
    --p2p-seed-node=seed.steem.fans:2001 \
    --p2p-seed-node=seed.justyy.com:2001 \
    --p2p-seed-node=seed.steemworld.org:2001 \
    --p2p-seed-node=seed.moecki.online:2001 \
    --p2p-seed-node=seed.pennsif.net:2001

echo "Test A started. Container: steem-test-a"

echo ""
echo "=== Downloading block_log for Test B ==="
if [ ! -f "$BASE/test-b/data/blockchain/block_log" ]; then
  echo "Downloading block_log (~381GB, this will take a while)..."
  wget -O "$BASE/test-b/data/blockchain/block_log" \
    https://steemit-dev-blockchainstate.s3.amazonaws.com/block_log-latest
  echo "block_log download complete"
else
  echo "block_log already exists, skipping download"
fi

echo ""
echo "=== Starting Test B (block_log replay with --force-validate) ==="
if docker ps -a --format '{{.Names}}' | grep -q '^steem-test-b$'; then
  echo "steem-test-b already exists, removing..."
  docker rm -f steem-test-b
fi

# Copy config for Test B
cp "$BASE/test-a/data/config.ini" "$BASE/test-b/data/config.ini"

docker run -d \
  --name steem-test-b \
  -v "$BASE/test-b/data:/var/steem" \
  -p 28091:8091 \
  -p 28090:8090 \
  -p 22001:2001 \
  --restart unless-stopped \
  "$IMAGE_NAME" \
  /usr/local/steemd/bin/steemd \
    --data-dir=/var/steem \
    --replay-blockchain \
    --force-validate \
    --p2p-seed-node=seed.steem.fans:2001 \
    --p2p-seed-node=seed.justyy.com:2001 \
    --p2p-seed-node=seed.steemworld.org:2001

echo "Test B started. Container: steem-test-b"

echo ""
echo "=== Both tests running ==="
echo "Monitor with:"
echo "  docker logs -f steem-test-a"
echo "  docker logs -f steem-test-b"
echo ""
echo "Check progress:"
echo "  curl -s -d '{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"database_api.get_dynamic_global_properties\"}' http://localhost:18091 | jq '{block: .result.head_block_number, time: .result.time}'"
echo "  curl -s -d '{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"database_api.get_dynamic_global_properties\"}' http://localhost:28091 | jq '{block: .result.head_block_number, time: .result.time}'"
