#include "Engine.h"
#include <regression/tframe.h>
#include <spine/Reactor.h>

using namespace std;

SmartMet::Engine::Gis::Engine *gengine;

namespace Tests
{
// ----------------------------------------------------------------------

void getBBox()
{
  using namespace SmartMet;

  auto bbox = gengine->getBBox(2393);
  if (bbox.west != 19.24)
    TEST_FAILED("YKJ west limit should be 19.24, not " + std::to_string(bbox.west));
  if (bbox.east != 31.59)
    TEST_FAILED("YKJ east limit should be 31.59, not " + std::to_string(bbox.east));
  if (bbox.south != 59.75)
    TEST_FAILED("YKJ south limit should be 59.75, not " + std::to_string(bbox.south));
  if (bbox.north != 70.09)
    TEST_FAILED("YKJ north limit should be 70.09, not " + std::to_string(bbox.north));

  // Undefined EPSG
  bbox = gengine->getBBox(666);
  if (bbox.west != -180)
    TEST_FAILED("EPSG:666 west limit should be -180");
  if (bbox.east != 180)
    TEST_FAILED("EPSG:666 east limit should be 180");
  if (bbox.south != -90)
    TEST_FAILED("EPSG:666 south limit should be -90");
  if (bbox.north != 90)
    TEST_FAILED("EPSG:666 north limit should be 90");

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void getEPSG()
{
  using namespace SmartMet;

  {
    auto epsg = gengine->getEPSG(3035);
    if (!epsg)
      TEST_FAILED("Failed to get EPSG information for EPSG:3035 (LAEA Europe)");

    if (epsg->number != 3035)
      TEST_FAILED("Got wrong ID for EPSG:3035");

    // Depends on EPSG version:
    if (epsg->name != "ETRS89 / LAEA Europe" && epsg->name != "ETRS89-extended / LAEA Europe")
      TEST_FAILED("Got wrong name for EPSG:3035 : '" + epsg->name + "'");

    if (epsg->scope.empty())
      TEST_FAILED("EPSG:3035 scope should not be empty, it should cover all Europe");

    if (epsg->deprecated)
      TEST_FAILED("EPSG:3035 should not be deprecated");
  }

  // Undefined EPSG

  {
    auto epsg = gengine->getEPSG(666);
    if (epsg)
      TEST_FAILED("Should not find EPSG:666");
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void getFeatures()
{
  SmartMet::Engine::Gis::MapOptions options;
  options.pgname = "";
  options.schema = "public";
  options.table = "varoalueet";
  options.fieldnames.insert("numero");

  auto features = gengine->getFeatures(options);

  for (const auto &feature : features)
  {
    if (!feature->geom || feature->geom->IsEmpty() != 0)
      TEST_FAILED("Encountered an empty geometry");

    try
    {
      const auto &value = feature->attributes.at("numero");
      if (value.which() != 0)
        TEST_FAILED("Expecting fied 'numero' values to be integers");
    }
    catch (...)
    {
      TEST_FAILED("Failed to read field 'numero'");
    }
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

// Test driver
class tests : public tframe::tests
{
  // Overridden message separator
  virtual const char *error_message_prefix() const { return "\n\t"; }
  // Main test suite
  void test()
  {
    TEST(getFeatures);
    TEST(getBBox);
    TEST(getEPSG);
  }
};  // class tests

}  // namespace Tests

int main(void)
{
  SmartMet::Spine::Options opts;
  opts.configfile = "cnf/reactor.conf";
  opts.parseConfig();

  SmartMet::Spine::Reactor reactor(opts);
  reactor.init();

  gengine = reinterpret_cast<SmartMet::Engine::Gis::Engine *>(reactor.getSingleton("Gis", NULL));
  cout << endl << "Engine tester" << endl << "=============" << endl;
  Tests::tests t;
  return t.run();
}
