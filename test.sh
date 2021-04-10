#!/bin/bash
set -e

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

images="
fedora:32
"

for img in ${images}; do
  docker run --rm -it --workdir=/root -v ${this_dir}:/s:ro,z \
    ${img} /bin/bash -c "/s/install-deps.sh && cmake /s && make -j5 && ./main"
done

