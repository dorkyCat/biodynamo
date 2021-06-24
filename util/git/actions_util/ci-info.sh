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

# This script is used to sample build/environment related information from our
# GitHub Actions runs. This information helps us determine the side-effects of
# installing and using BioDynaMo, and to create build recipes/guides.

_XML_OUT="$HOME/ci-run-info.xml"
_DIGIT_PAT='[[:digit:]]+.[[:digit:]]+.[[:digit:]]+'

function _err {
  printf "%.23s %s[%s]: %s\n" $(date +%F.%T.%N) ${BASH_SOURCE[1]##*/} ${BASH_LINENO[0]} "${@}"
}

# Description: Wrap string or stdin in given XML tag, append to _XML_OUT
# Usage: Tagged <tag-name> <str-or-stdin>
function Tagged {
  # pattern source: https://stackoverflow.com/a/12873723/13380539
  local escpat='s/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/"/\&quot;/g; s/'"'"'/\&#39;/g'
  local tag=$(echo "$1" | sed 's/[^[:alpha:]-]//g' | tr '[:upper:]' '[:lower:]' || echo 'error')
  local res=''
  local retc=0
  if (($# == 0)); then
    tag='error'
    retc=1
    echo 'Tag name required.'
    echo 'USAGE: Tagged <tag-name> <str-or-stdin>'
  elif (($# == 1)); then # pipe/redir
    res=$(sed "$escpat" </dev/stdin)
  else # 2nd string arg
    res=$(sed "$escpat" <<<"$2")
  fi
  echo "<$tag>$res</$tag>" >>"$_XML_OUT"
  return $retc
}

# Collect info BEFORE thisbdm using  
# commands common to most GNU/Linux systems
function CommonLinuxPreBdmInfoDump {
  uname -a | Tagged 'uname'
  Tagged 'os-release' </etc/os-release
  (
    set -o posix
    set
  ) | Tagged 'environment-pre-bdm'
  lsmod | Tagged 'modules-pre-bdm'
}

function CommonLinuxPyenvSetup {
  export PATH="$HOME/.pyenv/bin:$PATH"
  eval "$(pyenv init --path)"
  eval "$(pyenv init -)"
  pyenv shell 3.9.1
  which python3
}

# Collect info AFTER thisbdm
# Precondition: inside biodynamo build directory
function CommonLinuxBdmInfoDump {
  . bin/thisbdm.sh
  lsmod | Tagged 'modules-bdm'
  cmake --graphviz=dep.dot . && cat dep.dot | Tagged 'dependency-graph'
  rm dep.dot
  cmake --version | head -n 1 | grep -Eo "$_DIGIT_PAT" | Tagged 'cmake-version'
  cmake -LA -N . | awk '{if(f)print} /-- Cache values/{f=1}' | Tagged 'cmake-build-environment'
  biodynamo --version | Tagged 'bdm-version'
  mpiexec --version | head -n 1 | Tagged 'mpi-version'
  g++ --version | head -n 1 | Tagged 'compiler-version'
  python3 --version | grep -Eo "$_DIGIT_PAT" | Tagged 'python3-version'
  pip3 list --format=freeze --disable-pip-version-check | Tagged 'pip-packages'
}

# $1 os tag name attribute
function CompleteDumpXML {
  (
    echo '<?xml version="1.0"?>'
    echo "<os name='$1'>"
    cat "$_XML_OUT"
    echo '</os>'
  ) >"${_XML_OUT}.bak"
  rm "$_XML_OUT" && mv "${_XML_OUT}.bak" "$_XML_OUT"
}

function Centos7InfoDumpInit {
  touch "$_XML_OUT"
  yum list installed | Tagged 'packages-pre-bdm'
}

function Centos7InfoDump {
  CommonLinuxPreBdmInfoDump
  . scl_source enable devtoolset-8 || _err 'skip'
  . /etc/profile.d/modules.sh || _err 'skip'
  module load mpi || _err 'skip'
  CommonLinuxPyenvSetup
  export DISPLAY=:99.0
  sleep 3
  CommonLinuxBdmInfoDump
  # RPM specific commands
  yum list installed | Tagged 'packages-bdm'
  bash -c 'cat << EOF  > /etc/yum.repos.d/springdale-7-SCL.repo
[SCL-core]
name=Springdale SCL Base 7.6 - x86_64
mirrorlist=http://springdale.princeton.edu/data/springdale/SCL/7.6/x86_64/mirrorlist
#baseurl=http://springdale.princeton.edu/data/springdale/SCL/7.6/x86_64
gpgcheck=1
gpgkey=http://springdale.math.ias.edu/data/puias/7.6/x86_64/os/RPM-GPG-KEY-puias
EOF'
  echo 'repolist'
  yum repolist | Tagged 'repos-bdm'
  echo 'all deps'
  yum list -q -t -y available centos-release-scl epel-release wget \
    libXt-devel libXext-devel devtoolset-8-gcc* numactl-devel openmpi3-devel \
    freeglut-devel git @development zlib-devel bzip2 bzip2-devel \
    readline-devel sqlite sqlite-devel openssl-devel xz xz-devel libffi-devel \
    findutils doxygen graphviz valgrind freeglut-devel libxml2-devel \
    llvm-toolset-7 llvm-toolset-7-clang-tools-extra llvm-toolset-7-llvm-devel \
    llvm-toolset-7-llvm-static gdl-devel atlas-devel blas-devel lapack-devel |
    grep -E '(.noarch|.x86_64)' | Tagged 'package-manager-all-deps'
  # done
  CompleteDumpXML 'centos-7'
}
