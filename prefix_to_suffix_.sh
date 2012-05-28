#!/bin/bash
for f in "$@"
do
  sed -r -i.bk -e 's/(^|[^a-zA-Z0-9_])_(([0-9A-Za-z]+[_]?[0-9A-Za-z]+)+)/\1\2_/g' $f
done
