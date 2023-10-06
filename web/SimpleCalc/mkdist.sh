#!/bin/bash
set -eu

if [ -d files ]; then
    rm -r files
fi

mkdir files
cp -r build files/simple-calc
find files/simple-calc -maxdepth 3 -type d -name node_modules | xargs --no-run-if-empty rm -r

sed -i -E 's/SECCON\{.+\}/SECCON\{dummy\}/g' files/simple-calc/docker-compose.yml
sed -i -E 's/ADMIN_TOKEN=.+$/ADMIN_TOKEN=dummy/g' files/simple-calc/docker-compose.yml
