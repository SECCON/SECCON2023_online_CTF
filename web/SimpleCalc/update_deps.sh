#!/bin/bash
set -eu

pushd ./build/web/
npm update
popd

pushd ./build/bot/
npm update
popd

