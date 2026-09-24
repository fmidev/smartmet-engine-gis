#include "Engine.h"
#include <regression/tframe.h>
#include <spine/Reactor.h>
#include <string>
#include <vector>

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

// Identifier validation must reject request-influenceable schema/table
// names containing SQL metacharacters before any query is built. This
// runs entirely in the engine (no database access) as the check happens
// before db_connection().

void sqlInjectionRejected()
{
  const std::vector<std::string> evil = {
      "public; DROP TABLE varoalueet; --",
      "public\"; DROP TABLE x; --",
      "public.varoalueet",  // dot would collapse schema and table
      "public'",
      "1public",
      "",
      "public varoalueet"};

  for (const auto &bad : evil)
  {
    // Malicious schema name
    {
      SmartMet::Engine::Gis::MapOptions options;
      options.schema = bad;
      options.table = "varoalueet";
      bool threw = false;
      try
      {
        gengine->getFeatures(options);
      }
      catch (...)
      {
        threw = true;
      }
      if (!threw)
        TEST_FAILED("Malicious schema name was not rejected: '" + bad + "'");
    }

    // Malicious table name
    {
      SmartMet::Engine::Gis::MapOptions options;
      options.schema = "public";
      options.table = bad;
      bool threw = false;
      try
      {
        gengine->getFeatures(options);
      }
      catch (...)
      {
        threw = true;
      }
      if (!threw)
        TEST_FAILED("Malicious table name was not rejected: '" + bad + "'");
    }

    // Malicious time_column via getMetaData
    {
      SmartMet::Engine::Gis::MetaDataQueryOptions options;
      options.schema = "public";
      options.table = "varoalueet";
      options.geometry_column = "geom";
      options.time_column = bad;
      bool threw = false;
      try
      {
        gengine->getMetaData(options);
      }
      catch (...)
      {
        threw = true;
      }
      if (!threw)
        TEST_FAILED("Malicious time_column was not rejected: '" + bad + "'");
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
    TEST(sqlInjectionRejected);
    TEST(getFeatures);
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
