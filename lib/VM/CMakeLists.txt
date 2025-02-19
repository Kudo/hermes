# Copyright (c) Facebook, Inc. and its affiliates.
#
# This source code is licensed under the MIT license found in the LICENSE
# file in the root directory of this source tree.

set(source_files
  ArrayStorage.cpp
  BasicBlockExecutionInfo.cpp
  BuildMetadata.cpp
  Callable.cpp
  CellKind.cpp
  CheckHeapWellFormedAcceptor.cpp
  CodeBlock.cpp
  DictPropertyMap.cpp
  Domain.cpp
  GCBase.cpp
  GCCell.cpp
  OrderedHashMap.cpp
  HandleRootOwner.cpp
  HeapSnapshot.cpp
  HermesValue.cpp
  HiddenClass.cpp
  IdentifierTable.cpp
  Interpreter.cpp InstLayout.inc Interpreter-slowpaths.cpp
  JSArray.cpp
  JSArrayBuffer.cpp
  JSDataView.cpp
  JSDate.cpp
  JSError.cpp
  JSGenerator.cpp
  JSObject.cpp
  JSRegExp.cpp
  JSMapImpl.cpp
  JSTypedArray.cpp
  JSWeakMapImpl.cpp
  LimitedStorageProvider.cpp
  LogFailStorageProvider.cpp
  HostModel.cpp
  Operations.cpp
  PrimitiveBox.cpp
  Profiler.cpp
  Runtime.cpp Runtime-profilers.cpp
  RuntimeModule.cpp
  Profiler/ChromeTraceSerializerPosix.cpp
  Profiler/SamplingProfilerWindows.cpp
  Profiler/SamplingProfilerPosix.cpp
  SegmentedArray.cpp
  SerializedLiteralParser.cpp
  SingleObject.cpp
  StackFrame.cpp
  StorageProvider.cpp
  StringPrimitive.cpp
  StringView.cpp
  SymbolRegistry.cpp
  TimeLimitMonitor.cpp
  TwineChar16.cpp
  StringRefUtils.cpp
  VTable.cpp
  Metadata.cpp

  detail/IdentifierHashTable.cpp

  JSLib/Array.cpp
  JSLib/ArrayBuffer.cpp
  JSLib/ArrayIterator.cpp
  JSLib/DataView.cpp
  JSLib/TypedArray.cpp
  JSLib/Error.cpp
  JSLib/GeneratorFunction.cpp
  JSLib/GeneratorPrototype.cpp
  JSLib/GlobalObject.cpp
  JSLib/IteratorPrototype.cpp
  JSLib/HermesInternal.cpp
  JSLib/JSLibInternal.cpp JSLib/JSLibInternal.h
  JSLib/RuntimeCommonStorage.cpp
  JSLib/Map.cpp
  JSLib/Math.cpp
  JSLib/JSON.cpp
  JSLib/RuntimeJSONUtils.cpp
  JSLib/JSONLexer.cpp
  JSLib/Object.cpp
  JSLib/Set.cpp
  JSLib/String.cpp
  JSLib/StringIterator.cpp
  JSLib/Function.cpp
  JSLib/Number.cpp
  JSLib/Boolean.cpp
  JSLib/RegExp.cpp
  JSLib/DateUtil.cpp
  JSLib/Sorting.cpp
  JSLib/Symbol.cpp
  JSLib/Date.cpp JSLib/DateUtil.cpp
  JSLib/WeakMap.cpp
  JSLib/WeakSet.cpp
  JSLib/print.cpp
  JSLib/eval.cpp
  JSLib/escape.cpp
  JSLib/require.cpp
)

set(jit_files
  JIT/PoolHeap.cpp
  JIT/ExecHeap.cpp
  JIT/LLVMDisassembler.cpp
  JIT/NativeDisassembler.cpp
  JIT/DiscoverBB.cpp
  JIT/x86-64/JIT.cpp
  JIT/x86-64/FastJIT.cpp JIT/x86-64/FastJIT.h
  JIT/ExternalCalls.cpp JIT/ExternalCalls.h
  )

set(LLVM_OPTIONAL_SOURCES
  gcs/DependentMemoryRegion.cpp
  gcs/FillerCell.cpp
  gcs/GenGC.cpp
  gcs/MallocGC.cpp
  gcs/MarkBitArray.cpp
  gcs/OldGen.cpp
  gcs/Space.cpp
  gcs/YoungGen.cpp
  gcs/GCGeneration.cpp
  gcs/GCSegmentAddressIndex.cpp
  gcs/GenGCNC.cpp
  gcs/MarkBitArrayNC.cpp
  gcs/OldGenNC.cpp
  gcs/OldGenSegmentRanges.cpp
  gcs/YoungGenNC.cpp
  gcs/AlignedHeapSegment.cpp
  gcs/AlignedStorage.cpp
  gcs/CardTableNC.cpp
  ${jit_files}
)

if (${HERMESVM_GCKIND} STREQUAL "GENERATIONAL")
  list(APPEND source_files detail/gcs/BackingStorage.cpp gcs/CardObjectTable.cpp
                           gcs/CardTable.cpp gcs/CompleteMarkState.cpp
                           gcs/DependentMemoryRegion.cpp gcs/FillerCell.cpp
                           gcs/GenGC.cpp gcs/MarkBitArray.cpp gcs/OldGen.cpp
                           gcs/Space.cpp gcs/YoungGen.cpp)
elseif (${HERMESVM_GCKIND} STREQUAL "NONCONTIG_GENERATIONAL")
  list(APPEND source_files gcs/AlignedHeapSegment.cpp gcs/AlignedStorage.cpp
                           gcs/CardTableNC.cpp gcs/FillerCell.cpp
                           gcs/CompleteMarkState.cpp gcs/GCGeneration.cpp
                           gcs/GCSegmentAddressIndex.cpp gcs/GenGCNC.cpp
                           gcs/MarkBitArrayNC.cpp gcs/OldGenNC.cpp
                           gcs/OldGenSegmentRanges.cpp gcs/YoungGenNC.cpp)
elseif (${HERMESVM_GCKIND} STREQUAL "MALLOC")
  list(APPEND source_files gcs/MallocGC.cpp gcs/FillerCell.cpp)
else()
  message(WARNING "Not linking garbage collector")
endif()

list(APPEND source_files
  Debugger/Debugger.cpp
  JSLib/DebuggerInternal.cpp
)

# HostModel.cpp defines an abstract base class HostObjectProxy.
# This can be (and is) implemented by code which uses rtti, and
# therefore expects the base class to have typeinfo, so
# HostModel.cpp must be compiled -frtti, unlike most of the VM.
set_source_files_properties(HostModel.cpp PROPERTIES COMPILE_FLAGS -frtti)

if(HERMESVM_JIT)
  list(APPEND source_files ${jit_files})

  set(LLVM_LINK_COMPONENTS
    AllTargetsAsmPrinters
    AllTargetsAsmParsers
    AllTargetsDescs
    AllTargetsDisassemblers
    AllTargetsInfos
    MC
    MCParser
  )
endif()

add_llvm_library(hermesVMRuntime STATIC ${source_files}
    LINK_LIBS
    hermesInst
    hermesInstrumentation
    hermesSupport
    hermesParser
    hermesPlatform
    hermesHBCBackend
    dtoa
    ${CORE_FOUNDATION}
)

hermes_link_icu(hermesVMRuntime)

add_subdirectory(Instrumentation)
