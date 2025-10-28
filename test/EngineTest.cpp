#include "Engine.h"
#include <regression/tframe.h>
#include <spine/Reactor.h>

using namespace std;

std::shared_ptr<SmartMet::Engine::Gis::Engine> gengine;

namespace Tests
{
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
      if (value.index() != 0)
        TEST_FAILED("Expecting find 'numero' values to be integers");
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
  void test() { TEST(getFeatures); }
};  // class tests

}  // namespace Tests

int main(void)
{
  SmartMet::Spine::Options opts;
  opts.configfile = "cnf/reactor.conf";
  opts.parseConfig();

  SmartMet::Spine::Reactor reactor(opts);
  reactor.init();

  gengine = reactor.getEngine<SmartMet::Engine::Gis::Engine>("Gis", NULL);
  cout << endl
       << "Engine tester\n"
          "============="
       << endl;
  Tests::tests t;
  auto result = t.run();
  gengine.reset();
  return result;
}
