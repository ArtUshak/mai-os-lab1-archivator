#!/bin/sh

BUILD_TARGET=debug make

./bin/debug/anchorfield pack --input . --output ../output.anchorfield --verbose

./bin/debug/anchorfield list --input ../output.anchorfield

rm -rf ../output/
mkdir -p ../output/
./bin/debug/anchorfield unpack --input ../output.anchorfield --output ../output --verbose
