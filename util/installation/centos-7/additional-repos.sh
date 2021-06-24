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

bash -c 'cat << EOF  > /etc/yum.repos.d/springdale-7-SCL.repo
[SCL-core]
name=Springdale SCL Base 7.6 - x86_64
mirrorlist=http://springdale.princeton.edu/data/springdale/SCL/7.6/x86_64/mirrorlist
#baseurl=http://springdale.princeton.edu/data/springdale/SCL/7.6/x86_64
gpgcheck=1
gpgkey=http://springdale.math.ias.edu/data/puias/7.6/x86_64/os/RPM-GPG-KEY-puias
EOF'
