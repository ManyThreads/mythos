/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MIT License -- MyThOS: The Many-Threads Operating System
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Copyright 2016 Randolf Rotta, Robert Kuban, and contributors, BTU Cottbus-Senftenberg
 */
#include "plugins/test-caps.hh"

#include "cpu/hwthreadid.hh"
#include "objects/IKernelObject.hh"
#include "objects/ops.hh"
#include "objects/Example.hh"
#include "objects/KernelMemory.hh"
#include "objects/InvocationMock.hh"

namespace mythos {

extern CapEntry* getRootCapEntry();
extern KernelMemory* kmem_root();

namespace test_caps {

TestCaps instance;

TestCaps::TestCaps()
  : TestPlugin("test cap")
  , root_cap(*getRootCapEntry())
  , test_caps(image2kernel(_caps))
  , op(monitor)
{}

void TestCaps::printCaps()
{
  _logger.detail("root:", &root_cap, root_cap, DMDUMP(&root_cap, sizeof(mythos::CapEntry)));
  for (uint8_t i = 0; i < 10; ++i) {
    _logger.detail("cap", i, &test_caps[i], test_caps[i], DMDUMP(&test_caps[i], sizeof(mythos::CapEntry)));
  }
}

void TestCaps::initGlobal()
{

  _logger.error("derive from empty cap");
  TEST_FAILED(cap::derive(test_caps[0], test_caps[1], test_caps[0].cap()).state());

  _logger.error("derived from original");
  TEST_EQ(cap::derive(root_cap, test_caps[0], root_cap.cap(), ~CapRequest()).state(), Error::INVALID_REQUEST);
  TEST_SUCCESS(cap::derive(root_cap, test_caps[0], root_cap.cap()).state());

  _logger.error("reference from original");
  TEST_FAILED(cap::reference(root_cap, test_caps[0], root_cap.cap()).state());
  TEST_SUCCESS(cap::reference(root_cap, test_caps[1], root_cap.cap()).state());

  _logger.error("derived from derived");
  TEST_FAILED(cap::derive(test_caps[0], test_caps[2], test_caps[0].cap()).state());

  _logger.error("derived from reference");
  TEST_SUCCESS(cap::derive(test_caps[1], test_caps[3], test_caps[1].cap()).state());

  _logger.error("reference derived from derived");
  TEST_SUCCESS(cap::reference(test_caps[0], test_caps[4], test_caps[0].cap()).state());

  _logger.error("derived from derived reference");
  TEST_FAILED(cap::derive(test_caps[4], test_caps[5], test_caps[4].cap()).state());

  printCaps();
  _logger.error("move cap 4 into entry 2");
  TEST_FAILED(test_caps[1].acquire().state());
  TEST_SUCCESS(test_caps[2].acquire().state());
  TEST_SUCCESS(test_caps[4].moveTo(test_caps[2]).state());

  printCaps();

  ExampleFactory fac;
  _logger.error("factory test");
  TEST_FAILED(fac.factory(Cap(), kmem_root(), InvocationMock(test_caps[0], test_caps[4].cap()).addr(), &test_caps[5]).state());
  TEST_SUCCESS(fac.factory(Cap(), kmem_root(), InvocationMock(test_caps[0]).addr(), &test_caps[4]).state());
  TEST_FAILED(fac.factory(Cap(), kmem_root(), InvocationMock(test_caps[0], test_caps[4].cap()).addr(), &test_caps[5]).state());
  TEST_SUCCESS(fac.factory(Cap(), kmem_root(), InvocationMock(test_caps[3]).addr(), &test_caps[5]).state());
  TEST_SUCCESS(fac.factory(Cap(), kmem_root(), InvocationMock(test_caps[2]).addr(), &test_caps[6]).state());
  printCaps();

  _logger.error("derive some example obj reference");
  TEST_SUCCESS(cap::derive(test_caps[6], test_caps[7], test_caps[6].cap()).state());
  printCaps();
}

void TestCaps::initThread(size_t id)
{
  if (id == cpu::enumerateHwThreadID(0)) {
    state = 0;
    proto();
  }
}

void TestCaps::response(Tasklet*, optional<void> res)
{
  TEST_SUCCESS(res.state());
  proto();
}
void TestCaps::proto()
{
  switch (state++) {
    case 0:
      _logger.error("revoke cap 3");
      op.revokeCap(&tasklet, this, test_caps[3], nullptr);
      return;
    case 1:
      TEST_EQ(test_caps[5].cap(), Cap());
      _logger.error("delete cap 0");
      op.deleteCap(&tasklet, this, test_caps[0], nullptr);
      return;
    case 2:
      TEST_EQ(test_caps[0].cap(), Cap());
      TEST_EQ(test_caps[4].cap(), Cap());
      TEST_EQ(test_caps[6].cap(), Cap());
      printCaps();
  }
  done();
}

} // test_caps
} // mythos

