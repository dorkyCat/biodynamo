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

XML_OUT="$HOME/ci-run-info.xml"

# DESCRIPTION: Wrap string or stdin in given XML tag, append to XML_OUT
# USAGE: WrapInTag <tag-name> <str-or-stdin>
function WrapInTag {
    # pattern source: https://stackoverflow.com/a/12873723/13380539
    local escpat='s/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/"/\&quot;/g; s/'"'"'/\&#39;/g'
    local tag=$(echo "$1" | sed 's/[^[:alpha:]-]//g' | tr '[:upper:]' '[:lower:]' || echo 'error')
    local res=''
    local retc=0
    if (($# == 0)); then
        tag='error'
        retc=1
        echo 'Tag name required.'
        echo 'USAGE: WrapInTag <tag-name> <str-or-stdin>'
    elif (( $# == 1 )); then # pipe/redir
        res=$(sed "$escpat" < /dev/stdin)
    else # 2nd string arg
        res=$(sed "$escpat" <<< "$2")
    fi
    echo "<$tag>$res</$tag>" >> "$XML_OUT"
    return $retc
}

function Centos7InfoDumpInit {
    touch "$XML_OUT"
    yum list installed | WrapInTag 'packages-pre-bdm'
    return 0
}

function Centos7InfoDump {
    uname -a | WrapInTag 'uname'
    WrapInTag 'os-release' < /etc/os-release
    lsmod | WrapInTag 'modules-pre-bdm'
    ( set -o posix ; set ) | WrapInTag 'environment-raw'
    . scl_source enable devtoolset-8 || echo 'skip'
    . /etc/profile.d/modules.sh || echo 'skip'
    module load mpi || echo 'skip'
    export PATH="$HOME/.pyenv/bin:$PATH"
    eval "$(pyenv init --path)"
    eval "$(pyenv init -)"
    pyenv shell 3.9.1
    cmake --graphviz=dep.dot . && cat dep.dot | WrapInTag 'dependency-graph'
    ( set -o posix; set ) | WrapInTag 'environment-pre-bdm'
    . bin/thisbdm.sh
    export DISPLAY=:99.0
    sleep 3
    lsmod | WrapInTag 'modules-bdm'
    ( set -o posix; set ) | WrapInTag 'environment-bdm'
    yum list installed | WrapInTag 'packages-bdm'
    yum repolist | WrapInTag 'repos-bdm'
    cmake --version | head -n 1 | grep -Po '\d+.\d+.\d+' | WrapInTag 'cmake-version'
    cmake -LA -N . | awk '{if(f)print} /-- Cache values/{f=1}' | WrapInTag 'cmake-build-environment'
    biodynamo --version | WrapInTag 'bdm-version'
    mpiexec --version | head -n 1 | WrapInTag 'mpi-version'
    g++ --version | WrapInTag 'compiler-version'
    root --version 2>&1 | grep -Po '\d+.\d+/.*' | head -n 1 | WrapInTag 'root-version'
    python3 --version | grep -Po '\d+.\d.\d' | WrapInTag 'python3-version'
    paraview --version | grep -Po '\d+.\d+.\d+' | WrapInTag 'paraview-version'
    pip3 list --format=freeze --disable-pip-version-check | WrapInTag 'pip-packages'
    ( echo "<os version='centos-7'>"; cat "$XML_OUT"; echo "</os>" ) > "$XML_OUT"
    return 0
}

