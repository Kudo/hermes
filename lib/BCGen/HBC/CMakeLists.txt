# Copyright (c) Facebook, Inc. and its affiliates.
#
# This source code is licensed under the MIT license found in the LICENSE
# file in the root directory of this source tree.

add_llvm_library(hermesHBCBackend
  STATIC
  BackendContext.cpp
  HBC.cpp
  ISel.cpp
  Bytecode.cpp
  BytecodeStream.cpp
  BytecodeGenerator.cpp
  BytecodeDataProvider.cpp
  BytecodeProviderFromSrc.cpp
  BytecodeDisassembler.cpp
  BytecodeFormConverter.cpp
  ConsecutiveStringStorage.cpp
  DebugInfo.cpp
  Passes.cpp
  PredefinedStringIDs.cpp
  SerializedLiteralGenerator.cpp
  SerializedLiteralParserBase.cpp
  SimpleBytecodeBuilder.cpp
  TraverseLiteralStrings.cpp
  UniquingFilenameTable.cpp
  UniquingStringLiteralTable.cpp
  Passes/FuncCallNOpts.cpp
  Passes/InsertProfilePoint.cpp
  Passes/LowerBuiltinCalls.cpp
  Passes/OptEnvironmentInit.cpp
  LINK_LIBS
  hermesBackend
  hermesInst
  hermesSourceMap
  hermesAST
)
