#!/bin/bash
# -----------------------------------------------------------------------------
#
# Copyright (C) 2021 CERN & Newcastle University for the benefit of the
# BioDynaMo collaboration. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
#
# See the LICENSE file distributed with this work for details.
# See the NOTICE file distributed with this work for additional information
# regarding copyright ownership.
#
# -----------------------------------------------------------------------------

# This script installs the required packages
if [[ $# -ne 1 ]]; then
  echo "ERROR: Wrong number of arguments.
Description:
    This script installs the prerequisites of BioDynaMo, but not BioDynaMo
    itself. Script install.sh installs both prerequisites and BioDynaMo.
Arguments:
    <install_type>  all/required. If all is specified, then this script
                    will install all the prerequisites."
  exit 1
fi

BDM_PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../../.."

sudo -v

sudo yum update -y

# Install repositories
sudo yum install -y https://centos7.iuscommunity.org/ius-release.rpm \
  epel-release centos-release-scl

# Install required packages
sudo yum install -y \
  $(cat $BDM_PROJECT_DIR/util/installation/centos-7/package_list_required)

curl -L -O https://github.com/Kitware/CMake/releases/download/v3.19.3/cmake-3.19.3-Linux-x86_64.sh
chmod +x cmake-3.19.3-Linux-x86_64.sh
./cmake-3.19.3-Linux-x86_64.sh --skip-license --prefix=/usr/local

if [ -n "${PYENV_ROOT}" ]; then
  unset PYENV_ROOT
fi

# If PyEnv is not installed, install it
if [ ! -f "$HOME/.pyenv/bin/pyenv" ]; then
  echo "PyEnv was not found. Installing now..."
  curl https://pyenv.run | bash
fi
export PYENV_ROOT="$HOME/.pyenv"
export PATH="$PYENV_ROOT/bin:$PATH"
eval "$(pyenv init --path)"
eval "$(pyenv init -)"
pyenv update

PYVERS=3.9.1

# If Python $PYVERS is not installed, install it
if [ ! -f  "$HOME/.pyenv/versions/$PYVERS/lib/libpython3.so" ]; then
  echo "Python $PYVERS was not found. Installing now..."
  /usr/bin/env PYTHON_CONFIGURE_OPTS="--enable-shared" pyenv install -f $PYVERS
fi
pyenv shell $PYVERS

# Install optional packages
if [ $1 == "all" ]; then
  # Don't install --user: the packages should end up in the PYENV_ROOT directory
  python -m pip install $(cat $BDM_PROJECT_DIR/util/installation/common/pip_list_extra)
  # SBML integration
  ./$BDM_PROJECT_DIR/util/installation/centos-7/additional-repos.sh
  sudo yum install -y --nogpgcheck \
    $(cat $BDM_PROJECT_DIR/util/installation/centos-7/package_list_extra)
fi

exit 0
