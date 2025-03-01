// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "Mocking.h"
#include "TestClasses.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"

#include <catch_amalgamated.hpp>

using namespace o2;
using namespace o2::framework;

namespace o2::aod
{
O2HASH("TestA/0");
namespace test
{
DECLARE_SOA_COLUMN(X, x, float);
DECLARE_SOA_COLUMN(Y, y, float);
DECLARE_SOA_COLUMN(Z, z, float);
DECLARE_SOA_COLUMN(Foo, foo, float);
DECLARE_SOA_COLUMN(Bar, bar, float);
DECLARE_SOA_COLUMN(EventProperty, eventProperty, float);
DECLARE_SOA_DYNAMIC_COLUMN(Sum, sum, [](float x, float y) { return x + y; });
DECLARE_SOA_EXPRESSION_COLUMN(Sqfoo, sqfoo, float, nsqrt(test::foo));
} // namespace test

DECLARE_SOA_TABLE(Foos, "AOD", "FOO",
                  test::Foo);
DECLARE_SOA_EXTENDED_TABLE(Fooss, Foos, "FOOS", 0, test::Sqfoo);
DECLARE_SOA_TABLE(Bars, "AOD", "BAR",
                  test::Bar);
DECLARE_SOA_TABLE(FooBars, "AOD", "FOOBAR",
                  test::Foo, test::Bar,
                  test::Sum<test::Foo, test::Bar>);
DECLARE_SOA_TABLE(XYZ, "AOD", "XYZ",
                  test::X, test::Y, test::Z);
DECLARE_SOA_TABLE(Events, "AOD", "EVENTS",
                  test::EventProperty);

DECLARE_SOA_TABLE(Roots, "AOD", "ROOTS", test::Foo);

namespace idx
{
DECLARE_SOA_INDEX_COLUMN(Root, root);
}

DECLARE_SOA_TABLE(B1s, "AOD", "B1", idx::RootId, test::X);
DECLARE_SOA_TABLE(B2s, "AOD", "B2", idx::RootId, test::Y);
DECLARE_SOA_TABLE(B3s, "AOD", "B3", idx::RootId, test::Z);

namespace idx
{
DECLARE_SOA_INDEX_COLUMN(B1, b1);
DECLARE_SOA_INDEX_COLUMN(B2, b2);
DECLARE_SOA_INDEX_COLUMN(B3, b3);
} // namespace idx

DECLARE_SOA_INDEX_TABLE(Bs, Roots, "BS", idx::RootId, idx::B1Id, idx::B2Id, idx::B3Id);

} // namespace o2::aod

struct ATask {
  Produces<aod::FooBars> foobars;

  void process(o2::aod::Track const&)
  {
  }
};

struct ATaskconsumer {
  Spawns<aod::Fooss> foos;
  Builds<aod::Bs> bs;

  void init(InitContext&) {}
};

struct BTask {
  void process(o2::aod::Collision const&, o2::soa::Join<o2::aod::Tracks, o2::aod::TracksExtra, o2::aod::TracksCov> const&, o2::aod::AmbiguousTracks const&, o2::aod::Calos const&, o2::aod::CaloTriggers const&)
  {
  }
};

struct CTask {
  void process(o2::aod::Collision const&, o2::aod::Tracks const&)
  {
  }
};

struct DTask {
  void process(o2::aod::Tracks const&)
  {
  }
};

struct ETask {
  void process(o2::aod::FooBars::iterator const& foobar)
  {
    foobar.sum();
  }
};

struct FTask {
  expressions::Filter fooFilter = aod::test::foo > 1.;
  void process(soa::Filtered<o2::aod::FooBars>::iterator const& foobar)
  {
    foobar.sum();
  }
};

struct GTask {
  void process(o2::soa::Join<o2::aod::Foos, o2::aod::Bars, o2::aod::XYZ> const& foobars)
  {
    for (auto foobar : foobars) {
      foobar.x();
      foobar.foo();
      foobar.bar();
    }
  }
};

struct HTask {
  void process(o2::soa::Join<o2::aod::Foos, o2::aod::Bars, o2::aod::XYZ>::iterator const& foobar)
  {
    foobar.x();
    foobar.foo();
    foobar.bar();
  }
};

struct ITask {
  expressions::Filter flt = aod::test::bar > 0.;
  void process(o2::aod::Collision const&, o2::soa::Filtered<o2::soa::Join<o2::aod::Foos, o2::aod::Bars, o2::aod::XYZ>> const& foobars)
  {
    for (auto foobar : foobars) {
      foobar.x();
      foobar.foo();
      foobar.bar();
    }
  }
};

struct JTask {
  Configurable<o2::test::SimplePODClass> cfg{"someConfigurable", {}, "Some Configurable Object"};
  void process(o2::aod::Collision const&)
  {
    REQUIRE(cfg->x == 1);
  }
};

struct TestCCDBObject {
  int SomeObject;
};

struct KTask {
  struct : public ConfigurableGroup {
    std::string prefix = "foo";
    Configurable<int> anInt{"someConfigurable", {}, "Some Configurable Object"};
    Configurable<int> anotherInt{"someOtherConfigurable", {}, "Some Configurable Object"};
  } foo;

  Configurable<int> anThirdInt{"someThirdConfigurable", {}, "Some Configurable Object"};
  struct : public ConditionGroup {
    Condition<TestCCDBObject> test{"path"};
  } conditions;
  std::unique_ptr<int> someInt;
  std::shared_ptr<int> someSharedInt;
};

struct LTask {
  SliceCache cache;
  Preslice<aod::Tracks> perCol = aod::track::collisionId;
  PresliceOptional<aod::Tracks> perPart = aod::mctracklabel::mcParticleId;
  PresliceUnsorted<aod::McCollisionLabels> perMcCol = aod::mccollisionlabel::mcCollisionId;
  PresliceUnsortedOptional<aod::Collisions> perMcColopt = aod::mccollisionlabel::mcCollisionId;
  void process(aod::McCollision const&, soa::SmallGroups<soa::Join<aod::Collisions, aod::McCollisionLabels>> const&) {}
};

TEST_CASE("AdaptorCompilation")
{
  auto cfgc = makeEmptyConfigContext();

  REQUIRE(brace_constructible_size<ATask>() == 1);
  auto task1ng = adaptAnalysisTask<ATask>(*cfgc, TaskName{"test1"});
  REQUIRE(task1ng.inputs.size() == 2);
  REQUIRE(task1ng.outputs.size() == 1);
  REQUIRE(task1ng.inputs[1].binding == std::string("TracksExtension"));
  REQUIRE(task1ng.inputs[0].binding == std::string("Tracks"));
  REQUIRE(task1ng.outputs[0].binding.value == std::string("FooBars"));

  auto task1ngc = adaptAnalysisTask<ATaskconsumer>(*cfgc);
  REQUIRE(task1ngc.inputs.size() == 5);
  REQUIRE(task1ngc.inputs[0].binding == "Foos");
  REQUIRE(task1ngc.inputs[1].binding == "Roots");
  REQUIRE(task1ngc.inputs[2].binding == "B1s");
  REQUIRE(task1ngc.inputs[3].binding == "B2s");
  REQUIRE(task1ngc.inputs[4].binding == "B3s");

  auto task2 = adaptAnalysisTask<BTask>(*cfgc, TaskName{"test2"});
  REQUIRE(task2.inputs.size() == 10);
  REQUIRE(task2.inputs[2].binding == "TracksExtension");
  REQUIRE(task2.inputs[1].binding == "Tracks");
  REQUIRE(task2.inputs[4].binding == "TracksExtra_002Extension");
  REQUIRE(task2.inputs[3].binding == "TracksExtra");
  REQUIRE(task2.inputs[6].binding == "TracksCovExtension");
  REQUIRE(task2.inputs[5].binding == "TracksCov");
  REQUIRE(task2.inputs[7].binding == "AmbiguousTracks");
  REQUIRE(task2.inputs[8].binding == "Calos");
  REQUIRE(task2.inputs[9].binding == "CaloTriggers");
  REQUIRE(task2.inputs[0].binding == "Collisions_001");

  auto task3 = adaptAnalysisTask<CTask>(*cfgc, TaskName{"test3"});
  REQUIRE(task3.inputs.size() == 3);
  REQUIRE(task3.inputs[0].binding == "Collisions_001");
  REQUIRE(task3.inputs[1].binding == "Tracks");
  REQUIRE(task3.inputs[2].binding == "TracksExtension");

  auto task4 = adaptAnalysisTask<DTask>(*cfgc, TaskName{"test4"});
  REQUIRE(task4.inputs.size() == 2);
  REQUIRE(task4.inputs[0].binding == "Tracks");
  REQUIRE(task4.inputs[1].binding == "TracksExtension");

  auto task5 = adaptAnalysisTask<ETask>(*cfgc, TaskName{"test5"});
  REQUIRE(task5.inputs.size() == 1);
  REQUIRE(task5.inputs[0].binding == "FooBars");

  auto task6ng = adaptAnalysisTask<FTask>(*cfgc, TaskName{"test6"});
  REQUIRE(task6ng.inputs.size() == 1);
  REQUIRE(task6ng.inputs[0].binding == "FooBars");

  auto task7ng = adaptAnalysisTask<GTask>(*cfgc, TaskName{"test7"});
  REQUIRE(task7ng.inputs.size() == 3);
  REQUIRE(task7ng.inputs[0].binding == "Foos");
  REQUIRE(task7ng.inputs[1].binding == "Bars");
  REQUIRE(task7ng.inputs[2].binding == "XYZ");

  auto task8ng = adaptAnalysisTask<HTask>(*cfgc, TaskName{"test8"});
  REQUIRE(task8ng.inputs.size() == 3);

  auto task9ng = adaptAnalysisTask<ITask>(*cfgc, TaskName{"test9"});
  REQUIRE(task9ng.inputs.size() == 4);

  auto task10 = adaptAnalysisTask<JTask>(*cfgc, TaskName{"test10"});
  REQUIRE(task10.inputs.size() == 1);

  auto task11 = adaptAnalysisTask<KTask>(*cfgc, TaskName{"test11"});
  REQUIRE(task11.options.size() == 3);
  REQUIRE(task11.inputs.size() == 1);

  auto task12 = adaptAnalysisTask<LTask>(*cfgc, TaskName{"test12"});
  REQUIRE(task12.inputs.size() == 3);
}

TEST_CASE("TestPartitionIteration")
{
  TableBuilder builderA;
  auto rowWriterA = builderA.persist<float, float>({"fX", "fY"});
  rowWriterA(0, 0.0f, 8.0f);
  rowWriterA(0, 1.0f, 9.0f);
  rowWriterA(0, 2.0f, 10.0f);
  rowWriterA(0, 3.0f, 11.0f);
  rowWriterA(0, 4.0f, 12.0f);
  rowWriterA(0, 5.0f, 13.0f);
  rowWriterA(0, 6.0f, 14.0f);
  rowWriterA(0, 7.0f, 15.0f);
  auto tableA = builderA.finalize();
  REQUIRE(tableA->num_rows() == 8);

  using TestA = soa::InPlaceTable<"TestA/0"_h, o2::soa::Index<>, aod::test::X, aod::test::Y>;
  using FilteredTest = o2::soa::Filtered<TestA>;
  using PartitionTest = Partition<TestA>;
  using PartitionFilteredTest = Partition<o2::soa::Filtered<TestA>>;
  using PartitionNestedFilteredTest = Partition<o2::soa::Filtered<o2::soa::Filtered<TestA>>>;
  using namespace o2::framework;

  TestA testA{tableA};

  PartitionTest p1 = aod::test::x < 4.0f;
  p1.bindTable(testA);
  REQUIRE(4 == p1.size());
  REQUIRE(p1.begin() != p1.end());
  auto i = 0;
  for (auto& p : p1) {
    REQUIRE(i == p.x());
    REQUIRE(i + 8 == p.y());
    REQUIRE(i == p.index());
    i++;
  }
  REQUIRE(i == 4);

  expressions::Filter f1 = aod::test::x < 4.0f;
  auto selection = expressions::createSelection(testA.asArrowTable(), f1);
  FilteredTest filtered{{testA.asArrowTable()}, o2::soa::selectionToVector(selection)};
  PartitionFilteredTest p2 = aod::test::y > 9.0f;
  p2.bindTable(filtered);

  REQUIRE(2 == p2.size());
  i = 0;
  for (auto& p : p2) {
    REQUIRE(i + 2 == p.x());
    REQUIRE(i + 10 == p.y());
    REQUIRE(i + 2 == p.index());
    i++;
  }
  REQUIRE(i == 2);

  PartitionNestedFilteredTest p3 = aod::test::x < 3.0f;
  p3.bindTable(*(p2.mFiltered));
  REQUIRE(1 == p3.size());
  i = 0;
  for (auto& p : p3) {
    REQUIRE(i + 2 == p.x());
    REQUIRE(i + 10 == p.y());
    REQUIRE(i + 2 == p.index());
    i++;
  }
  REQUIRE(i == 1);
}
