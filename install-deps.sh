#!/bin/bash

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ ! -x ${this_dir}/seastar/install-dependencies.sh ]; then
  echo "setup needed"
  exit 1
fi

${this_dir}/seastar/install-dependencies.sh
