/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MyThOS: The Many-Threads Operating System
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
#include "plugins/test-mem.hh"

#include "cpu/hwthreadid.hh"
#include "objects/IKernelObject.hh"
#include "objects/ops.hh"
#include "plugins/test-macros.hh"
#include "objects/StaticMemoryRegion.hh"
#include "objects/MemoryRegion.hh"
#include "objects/FrameData.hh"
#include "objects/UntypedMemory.hh"
#include "objects/PageMap.hh"

namespace mythos {

extern CapEntry* getRootCapEntry();
extern UntypedMemory* kmem_root();

namespace test_mem {

TestMem instance;

TestMem::TestMem()
  : TestPlugin("test mem")
  , caps(image2kernel(_caps))
  , op(monitor)
{}

void TestMem::printCaps()
{
  for (uint8_t i = 0; i < 20; ++i) {
    _logger.detail("cap", i, &caps[i], caps[i], DMDUMP(&caps[i], sizeof(mythos::CapEntry)));
  }
}

void TestMem::initGlobal()
{
  createRegions();
  createFrames();
  createMaps();
  printCaps();
  createMappings();
}

void TestMem::createRegions()
{
  _logger.error("create root cap for region 0..1");
  physical_memory[0].initRootCap(caps[0]);
  TEST_NEQ(caps[0].cap(), Cap());
  physical_memory[1].initRootCap(caps[1]);
  TEST_NEQ(caps[1].cap(), Cap());
  TEST_NEQ(caps[0].cap(), caps[1].cap());
  printCaps();

  _logger.error("create memory region from UM");
  TEST_SUCCESS(MemoryRegion::factory(getRootCapEntry(), getRootCapEntry()->cap(), kmem_root(), 16*4096, 4096, &caps[2]).state());

  _logger.error("reference memory region");
  TEST_SUCCESS(cap::reference(caps[0], caps[3], caps[0].cap()).state());
  TEST_FAILED(caps[3].cap().getPtr()->cast<IFrame>().state());

}

void TestMem::createFrames()
{
  _logger.error("derive frames from static memory");
  CapRequest req = REQUEST_1GB | DENY_WRITE | ((1ul << 20) << PAGE_OFFSET_OFFSET);
  TEST_SUCCESS(cap::derive(caps[0], caps[4], caps[0].cap(), req).state());
  { // 1GB frame on 4GB offset
    auto cap = caps[4].cap();
    auto framePtr = cap.getPtr()->cast<IFrame>();
    TEST_SUCCESS(framePtr.state());
    TEST_EQ(framePtr->getSize(cap), 1ul << 30);
    TEST_EQ(framePtr->getAddress(cap).physint(), 1ul << 32);
    TEST_FALSE(framePtr->isKernelMemory(cap));
  }

  req = REQUEST_4KB | DENY_EXECUTE | ((5 << 20) << PAGE_OFFSET_OFFSET);
  TEST_SUCCESS(cap::derive(caps[3], caps[5], caps[3].cap(), req).state());
  { // 4KB frame on 20GB offset
    auto cap = caps[5].cap();
    auto framePtr = cap.getPtr()->cast<IFrame>();
    TEST_SUCCESS(framePtr.state());
    TEST_EQ(framePtr->getSize(cap), 1ul << 12);
    TEST_EQ(framePtr->getAddress(cap).physint(), 5ul << 32);
    TEST_FALSE(framePtr->isKernelMemory(cap));
  }

  req = REQUEST_4KB | DENY_EXECUTE | (4 << PAGE_OFFSET_OFFSET);
  TEST_SUCCESS(cap::reference(caps[4], caps[6], caps[4].cap(), req).state());
  { // 4KB frame on 4GB + 16KB offset
    auto cap = caps[6].cap();
    auto framePtr = cap.getPtr()->cast<IFrame>();
    TEST_SUCCESS(framePtr.state());
    TEST_EQ(framePtr->getSize(cap), 1ul << 12);
    TEST_EQ(framePtr->getAddress(cap).physint(), (1ul << 32) + 4*FrameSize::MIN_FRAME_SIZE);
    TEST_FALSE(framePtr->isKernelMemory(cap));
  }

  _logger.error("derive frames from dynamic memory");
  req = REQUEST_4KB | (16 << PAGE_OFFSET_OFFSET);
  TEST_FAILED(cap::derive(caps[2], caps[7], caps[2].cap(), req).state());
  req = REQUEST_4KB | (15 << PAGE_OFFSET_OFFSET);
  TEST_SUCCESS(cap::derive(caps[2], caps[7], caps[2].cap(), req).state());
  { // 4KB frame on in kernel mem
    auto cap = caps[7].cap();
    auto framePtr = cap.getPtr()->cast<IFrame>();
    TEST_SUCCESS(framePtr.state());
    TEST_EQ(framePtr->getSize(cap), 1ul << 12);
    TEST_TRUE(framePtr->isKernelMemory(cap));
    TEST_TRUE(isKernelAddress(framePtr->getAddress(cap).log()));
  }
}

void TestMem::createMaps()
{
  _logger.error("create some maps from UM");
  TEST_SUCCESS(PML4Map::factory(getRootCapEntry(), getRootCapEntry()->cap(), kmem_root(), &caps[10]).state());
  {
    auto cap = caps[10].cap();
    auto map = cap.getPtr()->cast<IPageMap>();
    TEST_SUCCESS(map.state());
    pml4map = *map;
    TEST_EQ(pml4map->level(), 4);
    TEST_TRUE(pml4map->isRootMap());
    TEST_OP(pml4map->getTableAddress().physint(), <=, KERNELMEM_SIZE);
    TEST_TRUE(pml4map->isReadWriteable(caps[10].cap()));
    TEST_TRUE(pml4map->isExecutable(cap));
    TEST_TRUE(pml4map->isMappable(cap));
  }

  TEST_SUCCESS(PML3Map::factory(getRootCapEntry(), getRootCapEntry()->cap(), kmem_root(), &caps[11]).state());
  {
    auto cap = caps[11].cap();
    auto map = cap.getPtr()->cast<IPageMap>();
    TEST_SUCCESS(map.state());
    pml3map = *map;
    TEST_EQ(pml3map->level(), 3);
    TEST_FALSE(pml3map->isRootMap());
    TEST_OP(pml3map->getTableAddress().physint(), <=, KERNELMEM_SIZE);
    TEST_TRUE(pml3map->isReadWriteable(cap));
    TEST_TRUE(pml3map->isExecutable(cap));
    TEST_TRUE(pml3map->isMappable(cap));
  }

  TEST_SUCCESS(PML2Map::factory(getRootCapEntry(), getRootCapEntry()->cap(), kmem_root(), &caps[12]).state());
  {
    auto cap = caps[12].cap();
    auto map = cap.getPtr()->cast<IPageMap>();
    TEST_SUCCESS(map.state());
    pml2map = *map;
    TEST_EQ(pml2map->level(), 2);
    TEST_FALSE(pml2map->isRootMap());
    TEST_OP(pml2map->getTableAddress().physint(), <=, KERNELMEM_SIZE);
    TEST_TRUE(pml2map->isReadWriteable(cap));
    TEST_TRUE(pml2map->isExecutable(cap));
    TEST_TRUE(pml2map->isMappable(cap));
  }

  TEST_SUCCESS(PML1Map::factory(getRootCapEntry(), getRootCapEntry()->cap(), kmem_root(), &caps[13]).state());
  {
    auto cap = caps[13].cap();
    auto map = cap.getPtr()->cast<IPageMap>();
    TEST_SUCCESS(map.state());
    pml1map = *map;
    TEST_EQ(pml1map->level(), 1);
    TEST_FALSE(pml1map->isRootMap());
    TEST_OP(pml1map->getTableAddress().physint(), <=, KERNELMEM_SIZE);
    TEST_TRUE(pml1map->isReadWriteable(cap));
    TEST_TRUE(pml1map->isExecutable(cap));
    TEST_TRUE(pml1map->isMappable(cap));
  }
}

void TestMem::createMappings()
{
  _logger.error("map the tables");
   TEST_FAILED(pml4map->mapTable(caps[10].cap(), &caps[11], caps[11].cap(), WRITEABLE + EXECUTABLE, 256).state());
   TEST_SUCCESS(pml4map->mapTable(caps[10].cap(), &caps[11], caps[11].cap(), WRITEABLE + EXECUTABLE, 0).state());
   TEST_SUCCESS(pml3map->mapTable(caps[11].cap(), &caps[12], caps[12].cap(), WRITEABLE + EXECUTABLE, 0).state());
   TEST_SUCCESS(pml2map->mapTable(caps[12].cap(), &caps[13], caps[13].cap(), WRITEABLE + EXECUTABLE, 0).state());
  _logger.error("map a frames and do some lookup");
   TEST_SUCCESS(pml1map->mapFrame(caps[13].cap(), &caps[7], caps[7].cap(), WRITEABLE + EXECUTABLE, 0, 0).state());
   auto entry1 = pml1map->lookup(0);
   TEST_SUCCESS(entry1.state());
   auto entry2 = pml2map->lookup(0);
   TEST_SUCCESS(entry2.state());
   TEST_EQ(*entry1, *entry2);
   entry2 = pml3map->lookup(0);
   TEST_SUCCESS(entry2.state());
   TEST_EQ(*entry1, *entry2);
   entry2 = pml4map->lookup(0);
   TEST_SUCCESS(entry2.state());
   TEST_EQ(*entry1, *entry2);
}

void TestMem::initThread(size_t id)
{
  if (id == cpu::enumerateHwThreadID(0)) {
    state = 0;
    proto();
  }
}

void TestMem::response(Tasklet*, optional<void> res)
{
  TEST_SUCCESS(res.state());
  proto();
}
void TestMem::proto()
{
  switch (state++) {
    case 0:
      _logger.error("revoke reference (cap 3)");
      op.revokeCap(&tasklet, this, caps[3], nullptr);
      return;
    case 1:
      TEST_TRUE(caps[3].cap().isUsable());
      _logger.error("delete reference (cap 3)");
      op.deleteCap(&tasklet, this, caps[3], nullptr);
      return;
    case 2:
      TEST_EQ(caps[3].cap(), Cap());
      _logger.error("delete derived (cap 4)");
      op.deleteCap(&tasklet, this, caps[4], nullptr);
      return;
    case 3:
      TEST_EQ(caps[4].cap(), Cap());
      TEST_EQ(caps[6].cap(), Cap());
      _logger.error("delete dynamic MR");
      op.deleteCap(&tasklet, this, caps[2], nullptr);
      return;
    case 4:
      TEST_EQ(caps[2].cap(), Cap());
      TEST_EQ(caps[7].cap(), Cap());
      TEST_FAILED(pml4map->lookup(0).state());
      _logger.error("revoke root cap of static MR");
      op.revokeCap(&tasklet, this, caps[0], nullptr);
      return;
    case 5:
      TEST_TRUE(caps[0].cap().isUsable());
      TEST_EQ(caps[5].cap(), Cap());
  }
  printCaps();
  done();
}

} // test_caps
} // mythos

