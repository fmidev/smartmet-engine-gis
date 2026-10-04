#include "Engine.h"
#include "Normalize.h"
#include <gis/OGR.h>
#include <gis/SpatialReference.h>
#include <cmath>
#include <set>
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

namespace
{
SmartMet::Engine::Gis::MapOptions finland()
{
  SmartMet::Engine::Gis::MapOptions options;
  options.schema = "natural_earth";
  options.table = "admin_0_countries";
  options.where = "iso_a2='FI'";
  return options;
}

OGREnvelope envelope(const OGRGeometry& theGeom)
{
  OGREnvelope env;
  theGeom.getEnvelope(&env);
  return env;
}

int count_parts(const OGRGeometry& theGeom)
{
  const auto* collection = dynamic_cast<const OGRGeometryCollection*>(&theGeom);
  return collection ? collection->getNumGeometries() : 1;
}
}  // namespace

void getShape()
{
  // Without a spatial reference the shape is in the native WGS84 coordinates
  auto geom = gengine->getShape(nullptr, finland());
  if (!geom || geom->IsEmpty())
    TEST_FAILED("Finland should not be empty");
  auto env = envelope(*geom);
  if (std::abs(env.MinX - 20.6232) > 1e-3 || std::abs(env.MaxX - 31.5695) > 1e-3 ||
      std::abs(env.MinY - 59.8112) > 1e-3 || std::abs(env.MaxY - 70.0753) > 1e-3)
    TEST_FAILED("Finland envelope is wrong: " + std::to_string(env.MinX) + "," +
                std::to_string(env.MinY) + "," + std::to_string(env.MaxX) + "," +
                std::to_string(env.MaxY));

  // Projected to ETRS-TM35FIN
  Fmi::SpatialReference tm35("EPSG:3067");
  auto projected = gengine->getShape(&tm35, finland());
  if (!projected || projected->IsEmpty())
    TEST_FAILED("Projected Finland should not be empty");
  env = envelope(*projected);
  if (env.MinX < 0 || env.MaxX > 800000 || env.MinY < 6600000 || env.MaxY > 7800000)
    TEST_FAILED("Projected Finland envelope is outside the expected range: " +
                std::to_string(env.MinX) + "," + std::to_string(env.MinY) + "," +
                std::to_string(env.MaxX) + "," + std::to_string(env.MaxY));

  // The cached result must be identical
  auto again = gengine->getShape(nullptr, finland());
  if (Fmi::OGR::exportToWkt(*again) != Fmi::OGR::exportToWkt(*geom))
    TEST_FAILED("Repeated requests should return the same shape");

  // A where clause matching nothing gives an empty result
  auto options = finland();
  options.where = "iso_a2='XX'";
  auto nothing = gengine->getShape(nullptr, options);
  if (nothing && !nothing->IsEmpty())
    TEST_FAILED("A where clause matching nothing should give an empty shape");

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void getFeatureAttributes()
{
  SmartMet::Engine::Gis::MapOptions options;
  options.schema = "natural_earth";
  options.table = "admin_0_countries";
  options.where = "iso_a2 IN ('FI','SE','EE')";
  options.fieldnames.insert("iso_a2");
  options.fieldnames.insert("name");

  auto features = gengine->getFeatures(options);
  if (features.size() != 3)
    TEST_FAILED("Expected 3 countries, got " + std::to_string(features.size()));

  std::set<std::string> names;
  for (const auto& feature : features)
  {
    const auto& code = std::get<std::string>(feature->attributes.at("iso_a2"));
    const auto& name = std::get<std::string>(feature->attributes.at("name"));
    names.insert(code + ":" + name);
    if (!feature->geom || feature->geom->IsEmpty())
      TEST_FAILED("Empty geometry for " + name);
  }
  if (names != std::set<std::string>{"EE:Estonia", "FI:Finland", "SE:Sweden"})
    TEST_FAILED("Unexpected country attributes");

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void minarea()
{
  // Removing polygons smaller than 1000 km^2 drops small islands but keeps the mainland
  auto options = finland();
  auto full = gengine->getShape(nullptr, options);
  options.minarea = 1000;
  auto despeckled = gengine->getShape(nullptr, options);
  if (!despeckled || despeckled->IsEmpty())
    TEST_FAILED("The mainland should survive minarea=1000");
  if (count_parts(*despeckled) > count_parts(*full))
    TEST_FAILED("minarea should not add polygons");
  if (count_parts(*full) > 1 && count_parts(*despeckled) == count_parts(*full))
    TEST_FAILED("minarea=1000 should remove some of the " + std::to_string(count_parts(*full)) +
                " polygons");
  TEST_PASSED();
}

// ----------------------------------------------------------------------

void normalize()
{
  std::string str = "ÄÖå Helsinki";
  SmartMet::Engine::Gis::normalize_string(str);
  if (str != "aoa helsinki")
    TEST_FAILED("Expected 'aoa helsinki', got '" + str + "'");
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
    TEST(getShape);
    TEST(getFeatureAttributes);
    TEST(minarea);
    TEST(normalize);
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
