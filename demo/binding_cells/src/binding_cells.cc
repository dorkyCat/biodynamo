// -----------------------------------------------------------------------------
//
// Copyright (C) The BioDynaMo Project.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
//
// See the LICENSE file distributed with this work for details.
// See the NOTICE file distributed with this work for additional information
// regarding copyright ownership.
//
// -----------------------------------------------------------------------------

#include "binding_cells.h"
#include "core/parallel_execution/executor.h"

int main(int argc, const char** argv) {
  bdm::ParallelExecutor pe(argc, argv);
  return pe.Execute(bdm::Simulate);
}
