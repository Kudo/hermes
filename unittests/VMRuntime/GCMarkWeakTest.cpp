/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
#include "TestHelpers.h"
#include "gtest/gtest.h"
#include "hermes/VM/AllocResult.h"
#include "hermes/VM/GC.h"
#include "hermes/VM/GCPointer-inline.h"

#include <vector>

using namespace hermes::vm;

/// Testing weak marking support in whichever GC implementation the tests are
/// configured with.
///
/// Test uses GC::countUsedWeakRefs which doesn't exist in opt mode
#ifndef NDEBUG

namespace hermes {
namespace vm {

class TestCell final : public GCCell {
 public:
  static const VTable vt;
  int *numMarkWeakCalls;
  WeakRef<TestCell> weak;

  static bool classof(const GCCell *cell) {
    return cell->getKind() == CellKind::FillerCellKind;
  }

  static TestCell *create(DummyRuntime &runtime, int *numMarkWeakCalls) {
    return new (runtime.alloc(sizeof(TestCell)))
        TestCell(&runtime.getHeap(), numMarkWeakCalls);
  }

  static void _markWeakImpl(GCCell *cell, GC *gc) {
    auto *self = vmcast_during_gc<TestCell>(cell, gc);
    gc->markWeakRef(self->weak);
    ++*self->numMarkWeakCalls;
  }

  // Creates a cell that weakly references itself.
  TestCell(GC *gc, int *numMarkWeakCalls)
      : GCCell(gc, &vt), numMarkWeakCalls(numMarkWeakCalls), weak(gc, this) {}
};

const VTable TestCell::vt{CellKind::FillerCellKind,
                          sizeof(TestCell),
                          nullptr,
                          TestCell::_markWeakImpl};

} // namespace vm
} // namespace hermes

namespace {

MetadataTableForTests getMetadataTable() {
  // Nothing to mark for either of them, leave a blank metadata.
  static const Metadata storage[] = {Metadata(), Metadata()};
  return MetadataTableForTests(storage);
}

TEST(GCMarkWeakTest, MarkWeak) {
  int numMarkWeakCalls = 0;
  auto runtime = DummyRuntime::create(getMetadataTable(), kTestGCConfigSmall);
  DummyRuntime &rt = *runtime;
  auto &gc = rt.gc;
  // Probably zero, but we only care about the increase/decrease.
  const int initUsedWeak = gc.countUsedWeakRefs();

  GCCell *g = TestCell::create(rt, &numMarkWeakCalls);
  rt.pointerRoots.push_back(&g);
  gc.collect();

  TestCell *t = vmcast<TestCell>(g);
  ASSERT_TRUE(t->weak.isValid());
  HermesValue hv = t->weak.unsafeGetHermesValue();
  EXPECT_TRUE(t == hv.getObject());
  // Exactly one call to _markWeakImpl
  EXPECT_EQ(1, numMarkWeakCalls);
  EXPECT_EQ(initUsedWeak + 1, gc.countUsedWeakRefs());

  rt.pointerRoots.pop_back();
  gc.collect();
  // No new calls to _markWeakImpl
  EXPECT_EQ(1, numMarkWeakCalls);
  EXPECT_EQ(initUsedWeak, gc.countUsedWeakRefs());
}

} // namespace

#endif
