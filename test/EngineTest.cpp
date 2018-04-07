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
    TEST_FAILED("YKJ west limit should be 19.24");
  if (bbox.east != 31.59)
    TEST_FAILED("YKJ east limit should be 31.59");
  if (bbox.south != 59.75)
    TEST_FAILED("YKJ south limit should be 59.75");
  if (bbox.north != 70.09)
    TEST_FAILED("YKJ north limit should be 70.09");

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

// Test driver
class tests : public tframe::tests
{
  // Overridden message separator
  virtual const char *error_message_prefix() const { return "\n\t"; }
  // Main test suite
  void test() { TEST(getBBox); }
};  // class tests

}  // namespace Tests

int main(void)
{
  SmartMet::Spine::Options opts;
  opts.configfile = "cnf/reactor.conf";
  opts.parseConfig();

  SmartMet::Spine::Reactor reactor(opts);

  gengine = reinterpret_cast<SmartMet::Engine::Gis::Engine *>(reactor.getSingleton("Gis", NULL));
  cout << endl << "Engine tester" << endl << "=============" << endl;
  Tests::tests t;
  t.run();
}
