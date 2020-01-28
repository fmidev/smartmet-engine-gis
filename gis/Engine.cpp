// ======================================================================

#include "Engine.h"
#include "Config.h"
#include <boost/algorithm/string/join.hpp>
#include <gdal/gdal_version.h>
#include <gdal/ogrsf_frmts.h>
#include <gis/Box.h>
#include <gis/Host.h>
#include <gis/OGR.h>
#include <gis/PostGIS.h>
#include <macgyver/StringConversion.h>
#include <spine/Exception.h>
#include <list>
#include <memory>
#include <stdexcept>

auto featuredeleter = [](OGRFeature* p) { OGRFeature::DestroyFeature(p); };
using SafeFeature = std::unique_ptr<OGRFeature, decltype(featuredeleter)>;

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
namespace
{
int getEpsgCode(const GDALDataPtr& connection,
                const std::string& schema,
                const std::string& table,
                const std::string& geometry_column,
                const SmartMet::Engine::Gis::Config& config)
{
  try
  {
    auto layerdeleter = [&](OGRLayer* p) { connection->ReleaseResultSet(p); };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    std::string sqlStmt =
        "select st_srid(" + geometry_column + ") from " + schema + "." + table + " limit 1;";

    SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), nullptr, nullptr), layerdeleter);

    if (!pLayer)
      throw Spine::Exception(BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

    SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);

    if (pFeature)
      return pFeature->GetFieldAsInteger(0);

    if (config.getDefaultEPSG())
    {
      if (!config.quiet())
        std::cerr << "Warning: sqlStmt returned null. Setting EPSG to default value "
                  << *config.getDefaultEPSG() << std::endl;
      return *config.getDefaultEPSG();
    }

    throw Spine::Exception(BCP, "Gis-engine: Null feature received from " + sqlStmt);
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

GDALDataPtr db_connection(const Config& config, const std::string& pgname)
{
  try
  {
    // Read it from the database
    const postgis_connection_info& pgci = config.getPostGISConnectionInfo(pgname);

    Fmi::Host host(pgci.host, pgci.database, pgci.username, pgci.password, pgci.port);

    return host.connect();
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}
}  // namespace

// ----------------------------------------------------------------------
/*!
 * \brief Create cache-keys for the map options
 */
// ----------------------------------------------------------------------

std::pair<std::string, std::string> cache_keys(const MapOptions& theOptions,
                                               OGRSpatialReference* theSR)
{
  try
  {
    std::string wkt = "NULL";
    if (theSR)
    {
      char* tmp;
      theSR->exportToWkt(&tmp);
      wkt = tmp;
      OGRFree(tmp);  // Note: NOT delete!
    }

    std::string basic = theOptions.schema;
    basic += '|';
    basic += theOptions.table;
    basic += '|';
    basic += boost::algorithm::join(theOptions.fieldnames, "|");
    basic += '|';
    basic += (theOptions.where ? *theOptions.where : "");
    basic += '|';
    basic += wkt;

    std::string key = basic;
    key += '|';
    key += Fmi::to_string(theOptions.minarea ? *theOptions.minarea : 0.0);
    key += '|';
    key += Fmi::to_string(theOptions.mindistance ? *theOptions.mindistance : 0.0);

    return std::make_pair(basic, key);
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

OGREnvelope Engine::getTableEnvelope(const GDALDataPtr& connection,
                                     const std::string& schema,
                                     const std::string& table,
                                     const std::string& geometry_column,
                                     bool quiet) const
{
  try
  {
    // we assume there is just one geometry column
    auto fixed_bbox = itsConfig->getTableBBox(schema, table);
    if (fixed_bbox)
    {
      OGREnvelope bbox;
      bbox.MinX = fixed_bbox->west;
      bbox.MinY = fixed_bbox->south;
      bbox.MaxX = fixed_bbox->east;
      bbox.MaxY = fixed_bbox->north;
      return bbox;
    }

#if 0
    std::string sqlStmt = "SELECT ST_EstimatedExtent('" + schema + "', '" + table + "', '" +
                          geometry_column + "')::geometry as extent";
#else
    std::string sqlStmt = "SELECT ST_Extent(" + geometry_column + ")::geometry as extent FROM " +
                          schema + "." + table;
#endif

    auto layerdeleter = [&](OGRLayer* p) { connection->ReleaseResultSet(p); };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), nullptr, nullptr), layerdeleter);

    if (!pLayer)
      throw Spine::Exception(BCP, "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

    SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);

    if (!pFeature)
      throw Spine::Exception(BCP, "Gis-engine: PostGIS feature query failed: '" + sqlStmt + "'");

    // get geometry
    OGRGeometry* pGeometry = pFeature->GetGeometryRef();

    if (!pGeometry)
    {
      Spine::Exception ex(BCP, "Gis-engine: PostGIS feature query failed: '" + sqlStmt + "'");
      if (quiet)
        ex.disableLogging();
      throw ex;
    }

    OGREnvelope bbox;
    pGeometry->getEnvelope(&bbox);

    return bbox;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief The only permitted constructor requires a configfile
 */
// ----------------------------------------------------------------------

Engine::Engine(const std::string& theFileName) : itsConfigFile(theFileName), itsConfig() {}

// ----------------------------------------------------------------------
/*!
 * \brief Initialize the engine
 *
 * Note: We do not wish to delay the initialization of other engines.
 * init() is done in its own thread, hence we read the configuration
 * files here, and hence itsConfig is a pointer instead of an object.
 */
// ----------------------------------------------------------------------

void Engine::init()
{
  try
  {
    itsConfig.reset(new Config(itsConfigFile));

    itsCache.resize(itsConfig->getMaxCacheSize());
    itsFeaturesCache.resize(itsConfig->getMaxCacheSize());
    itsEnvelopeCache.resize(itsConfig->getMaxCacheSize());

    // Register all drivers just once

#if GDAL_VERSION_MAJOR < 2
    OGRRegisterAll();
#else
    GDALAllRegister();
#endif
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Shutdown the engine
 */
// ----------------------------------------------------------------------

void Engine::shutdown()
{
  std::cout << "  -- Shutdown requested (gis)\n";
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the CRS registry
 */
// ----------------------------------------------------------------------

CRSRegistry& Engine::getCRSRegistry()
{
  try
  {
    return itsConfig->getCRSRegistry();
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Fetch a shape from the database
 *
 * \param theOptions query options
 */
// ----------------------------------------------------------------------

OGRGeometryPtr Engine::getShape(OGRSpatialReference* theSR, const MapOptions& theOptions) const
{
  try
  {
    // Validate options
    if (theOptions.schema.empty())
      throw Spine::Exception(BCP, "PostGIS database name missing from map query");
    if (theOptions.table.empty())
      throw Spine::Exception(BCP, "PostGIS table name missing from map query");

    // Find simplified map from the cache

    auto keys = cache_keys(theOptions, theSR);
    const auto& basic_key = keys.first;
    const auto& full_key = keys.second;

    auto obj = itsCache.find(full_key);
    if (obj)
      return *obj;

    // Find full map from the cache

    OGRGeometryPtr geom;

    obj = itsCache.find(basic_key);
    if (obj)
      geom = *obj;
    else
    {
      // Read it from the database
      auto connection = db_connection(*itsConfig, theOptions.pgname);

      std::string name = theOptions.schema + "." + theOptions.table;
      geom = Fmi::PostGIS::read(theSR, connection, name, theOptions.where);

      // Cache the result if it's not empty
      if (geom)
        itsCache.insert(basic_key, geom);
    }

    // If no simplification was requested we're done
    if (!theOptions.minarea && !theOptions.mindistance)
      return geom;

    // Apply simplification options

    if (theOptions.minarea)
      geom.reset(Fmi::OGR::despeckle(*geom, *theOptions.minarea));

    if (theOptions.mindistance && geom)
    {
      const double kilometers_to_degrees = 1.0 / 110.0;  // one degree latitude =~ 110 km
      const double kilometers_to_meters = 1000;

      OGRSpatialReference* crs = geom->getSpatialReference();
      bool geographic = (crs ? crs->IsGeographic() : false);
      if (!geographic)
        geom.reset(
            geom->SimplifyPreserveTopology(kilometers_to_meters * (*theOptions.mindistance)));
      else
        geom.reset(
            geom->SimplifyPreserveTopology(kilometers_to_degrees * (*theOptions.mindistance)));
    }

    // Cache the result

    if (geom)
      itsCache.insert(full_key, geom);

    return geom;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

Fmi::Features Engine::getFeatures(OGRSpatialReference* theSR, const MapOptions& theOptions) const
{
  try
  {
    // Validate options
    if (theOptions.schema.empty())
      throw Spine::Exception(BCP, "PostGIS database name missing from map query");
    if (theOptions.table.empty())
      throw Spine::Exception(BCP, "PostGIS table name missing from map query");

    // Find simplified map from the cache
    auto keys = cache_keys(theOptions, theSR);
    const auto& basic_key = keys.first;
    const auto& full_key = keys.second;

    auto obj = itsFeaturesCache.find(full_key);
    if (obj)
      return *obj;

    Fmi::Features ret;

    // Find full map from the cache
    obj = itsFeaturesCache.find(basic_key);
    if (obj)
    {
      ret = *obj;
    }
    else
    {
      // Read it from the database
      auto connection = db_connection(*itsConfig, theOptions.pgname);

      std::string name = theOptions.schema + "." + theOptions.table;
      ret = Fmi::PostGIS::read(theSR, connection, name, theOptions.fieldnames, theOptions.where);

      // Cache the result if it's not empty
      if (!ret.empty())
        itsFeaturesCache.insert(basic_key, ret);
    }

    // If no simplification was requested we're done
    if (!theOptions.minarea && !theOptions.mindistance)
      return ret;

    // Apply simplification options

    Fmi::Features newfeatures;
    for (const auto& feature : ret)
    {
      Fmi::FeaturePtr newfeature = feature;
      if (theOptions.minarea)
        newfeature->geom.reset(Fmi::OGR::despeckle(*(newfeature->geom), *theOptions.minarea));

      if (theOptions.mindistance && newfeature->geom)
      {
        const double kilometers_to_degrees = 1.0 / 110.0;  // one degree latitude =~ 110 km
        const double kilometers_to_meters = 1000;

        OGRSpatialReference* crs = newfeature->geom->getSpatialReference();
        bool geographic = (crs ? crs->IsGeographic() : false);

        if (!geographic)
          newfeature->geom.reset(newfeature->geom->SimplifyPreserveTopology(
              kilometers_to_meters * (*theOptions.mindistance)));
        else
          newfeature->geom.reset(newfeature->geom->SimplifyPreserveTopology(
              kilometers_to_degrees * (*theOptions.mindistance)));
      }

      if (newfeature->geom)
        newfeatures.push_back(newfeature);
    }

    // Cache the result
    if (!newfeatures.empty())
      itsFeaturesCache.insert(full_key, newfeatures);

    return newfeatures;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Fetch meta data (timesteps and bounding box) from db
 *
 * \param theOptions query options
 */
// ----------------------------------------------------------------------

MetaData Engine::getMetaData(const MetaDataQueryOptions& theOptions) const
{
  try
  {
    MetaData metadata;

    auto connection = db_connection(*itsConfig, theOptions.pgname);

    // Get time always in UTC
    connection->ExecuteSQL("SET TIME ZONE UTC", nullptr, nullptr);

    auto layerdeleter = [&](OGRLayer* p) { connection->ReleaseResultSet(p); };
    using SafeLayer = std::unique_ptr<OGRLayer, decltype(layerdeleter)>;

    // 1) Get timesteps
    if (theOptions.time_column)
    {
      auto timestep = itsConfig->getTableTimeStep(theOptions.schema, theOptions.table);
      if (!timestep)
      {
        // List all available times
        std::string sqlStmt = "SELECT DISTINCT(" + *theOptions.time_column + ")" + " FROM " +
                              theOptions.schema + "." + theOptions.table + " WHERE " +
                              *theOptions.time_column + " IS NOT NULL ORDER by " +
                              *theOptions.time_column;

        SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), nullptr, nullptr), layerdeleter);

        if (!pLayer)
          throw Spine::Exception(BCP,
                                 "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

        while (true)
        {
          SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);
          if (!pFeature)
            break;

          tm timeinfo;

          bool ret = pFeature->GetFieldAsDateTime(0,
                                                  &timeinfo.tm_year,
                                                  &timeinfo.tm_mon,
                                                  &timeinfo.tm_mday,
                                                  &timeinfo.tm_hour,
                                                  &timeinfo.tm_min,
                                                  &timeinfo.tm_sec,
                                                  &timeinfo.tm_isdst);

          if (!ret)
          {
            std::cout << "Reading values from '" << theOptions.schema << "." << theOptions.table
                      << "." << *theOptions.time_column << "' failed!" << std::endl;
            break;
          }

          timeinfo.tm_year -= 1900;  // years after 1900
          timeinfo.tm_mon -= 1;      // months 0..11

          metadata.timesteps.push_back(boost::posix_time::ptime_from_tm(timeinfo));
        }
      }

      else
      {
        // Establish starttime and endtime
        std::string sqlStmt = "SELECT min(" + *theOptions.time_column + ") as mintime, max(" +
                              *theOptions.time_column + ") as maxtime FROM " + theOptions.schema +
                              "." + theOptions.table + " WHERE " + *theOptions.time_column +
                              " IS NOT NULL";

        SafeLayer pLayer(connection->ExecuteSQL(sqlStmt.c_str(), nullptr, nullptr), layerdeleter);

        if (!pLayer)
          throw Spine::Exception(BCP,
                                 "Gis-engine: PostGIS metadata query failed: '" + sqlStmt + "'");

        SafeFeature pFeature(pLayer->GetNextFeature(), featuredeleter);

        if (!pFeature)
          throw Spine::Exception(BCP,
                                 "Gis-engine: PostGIS feature query failed: '" + sqlStmt + "'");

        tm start_tm, end_tm;

        bool ret1 = pFeature->GetFieldAsDateTime(0,
                                                 &start_tm.tm_year,
                                                 &start_tm.tm_mon,
                                                 &start_tm.tm_mday,
                                                 &start_tm.tm_hour,
                                                 &start_tm.tm_min,
                                                 &start_tm.tm_sec,
                                                 &start_tm.tm_isdst);
        bool ret2 = pFeature->GetFieldAsDateTime(1,
                                                 &end_tm.tm_year,
                                                 &end_tm.tm_mon,
                                                 &end_tm.tm_mday,
                                                 &end_tm.tm_hour,
                                                 &end_tm.tm_min,
                                                 &end_tm.tm_sec,
                                                 &end_tm.tm_isdst);

        if (!ret1 || !ret2)
        {
          std::cout << "Reading values from '" << theOptions.schema << "." << theOptions.table
                    << "." << *theOptions.time_column << "' failed!" << std::endl;
        }
        else
        {
          start_tm.tm_year -= 1900;  // years after 1900
          start_tm.tm_mon -= 1;      // months 0..11

          end_tm.tm_year -= 1900;  // years after 1900
          end_tm.tm_mon -= 1;      // months 0..11

          auto starttime = boost::posix_time::ptime_from_tm(start_tm);
          auto endtime = boost::posix_time::ptime_from_tm(end_tm);
          metadata.timeinterval = TimeInterval{starttime, endtime, *timestep};
        }
      }
    }

    // 2) Get bounding box
    metadata.xmin = 0.0;
    metadata.ymin = 0.0;
    metadata.xmax = 0.0;
    metadata.ymax = 0.0;

    // Cache envelopes. We assume no need to reduce envelope sizes when old data is deleted
    auto hash = theOptions.hash_value();
    if (theOptions.time_column && !metadata.timesteps.empty())
    {
      const auto& last_time = metadata.timesteps.back();
      boost::hash_combine(hash, boost::hash_value(Fmi::to_iso_string(last_time)));
    }

    auto obj = itsEnvelopeCache.find(hash);
    if (obj)
    {
      metadata.xmin = obj->MinX;
      metadata.ymin = obj->MinY;
      metadata.xmax = obj->MaxX;
      metadata.ymax = obj->MaxY;
      return metadata;
    }

    OGREnvelope table_envelope = getTableEnvelope(connection,
                                                  theOptions.schema,
                                                  theOptions.table,
                                                  theOptions.geometry_column,
                                                  itsConfig->quiet());

    // convert source spatial reference system to EPSG:4326
    int epsg = getEpsgCode(
        connection, theOptions.schema, theOptions.table, theOptions.geometry_column, *itsConfig);

    OGRSpatialReference oSourceSRS;
    oSourceSRS.importFromEPSGA(epsg);
    OGRSpatialReference oTargetSRS;
    oTargetSRS.importFromEPSGA(4326);

    std::unique_ptr<OGRCoordinateTransformation> poCT(
        OGRCreateCoordinateTransformation(&oSourceSRS, &oTargetSRS));

    if (!poCT)
      throw Spine::Exception(BCP, "OGRCreateCoordinateTransformation function call failed");

    bool transformation_ok = (poCT->Transform(1, &(table_envelope.MinX), &(table_envelope.MinY)) &&
                              poCT->Transform(1, &(table_envelope.MaxX), &(table_envelope.MaxY)));
    if (transformation_ok)
    {
      metadata.xmin = table_envelope.MinX;
      metadata.ymin = table_envelope.MinY;
      metadata.xmax = table_envelope.MaxX;
      metadata.ymax = table_envelope.MaxY;
    }

    itsEnvelopeCache.insert(hash, table_envelope);

    return metadata;
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the bounding box for a EPSG
 */
// ----------------------------------------------------------------------

BBox Engine::getBBox(int theEPSG) const
{
  try
  {
    return itsConfig->getBBox(theEPSG);
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return EPSG information
 */
// ----------------------------------------------------------------------

boost::optional<EPSG> Engine::getEPSG(int theEPSG) const
{
  try
  {
    return itsConfig->getEPSG(theEPSG);
  }
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

void Engine::populateGeometryStorage(const PostGISIdentifierVector& thePostGISIdentifiers,
                                     GeometryStorage& theGeometryStorage) const
{
  try
  {
    // First Collect features with the same attribute value together.

    using FeatureList = std::list<const OGRGeometry*>;
    using FeatureStore = std::map<std::string, FeatureList>;

    FeatureStore featurestore;

    for (auto pgId : thePostGISIdentifiers)
    {
      if (itsShutdownRequested)
        return;

      std::string queryParam = pgId.key();
      if (theGeometryStorage.itsQueryParameters.find(queryParam) !=
          theGeometryStorage.itsQueryParameters.end())
        continue;

      theGeometryStorage.itsQueryParameters.insert(std::make_pair(queryParam, 1));

      SmartMet::Engine::Gis::MapOptions mo;
      mo.pgname = pgId.pgname;
      mo.schema = pgId.schema;
      mo.table = pgId.table;
      mo.fieldnames.insert(pgId.field);

      Fmi::Features features = getFeatures(nullptr, mo);

      for (auto feature : features)
      {
        const OGRGeometry* geom = feature->geom.get();

        if (!geom)
          continue;

        Fmi::Attribute attribute = feature->attributes.at(pgId.field);
        AttributeToString ats;
        std::string areaName = boost::apply_visitor(ats, attribute);

        // skip unknown features
        if (areaName.empty())
          continue;

        // convert to lower case
        boost::algorithm::to_lower(areaName);

        auto& featurelist = featurestore[areaName];  // creates empty list if necessary
        featurelist.push_back(geom);
      }
    }

    // Now if any named feature has more one than one component, compute a CascadedUnion
    // which should be much faster than performing an union two elements at a time. If
    // there is only one component, we keep it as is.

    for (auto& name_list : featurestore)
    {
      const auto& areaName = name_list.first;
      auto& featurelist = name_list.second;

      const OGRGeometry* geom = nullptr;

      if (featurelist.size() == 1)
        geom = featurelist.front();
      else
      {
        std::unique_ptr<OGRGeometryCollection> collection(new OGRGeometryCollection);
        for (auto geom_part : featurelist)
          collection->addGeometry(geom_part);
        geom = collection->UnionCascaded();
      }

      // Store based on type

      OGRwkbGeometryType geomType = geom->getGeometryType();

      if (theGeometryStorage.itsGeometries.find(geomType) == theGeometryStorage.itsGeometries.end())
      {
        // if that type of geometries not found, add new one
        NameOGRGeometryMap nameOGRGeometryMap;
        nameOGRGeometryMap.insert(
            make_pair(areaName, boost::shared_ptr<OGRGeometry>(geom->clone())));
        theGeometryStorage.itsGeometries.insert(std::make_pair(geomType, nameOGRGeometryMap));
      }
      else
      {
        // that type of geometries found
        NameOGRGeometryMap& nameOGRGeometryMap = theGeometryStorage.itsGeometries[geomType];
        nameOGRGeometryMap.insert(
            std::make_pair(areaName, boost::shared_ptr<OGRGeometry>(geom->clone())));
      }

      // Finally convert the geometries to SVG (for textgen??)

      std::string svgString = Fmi::OGR::exportToSvg(*geom, Fmi::Box::identity(), 6);

      if (geomType == wkbPolygon || geomType == wkbMultiPolygon)
      {
#ifdef MYDEBUG
        cout << "POLYGON in SVG format: " << svgString << endl;
#endif
        theGeometryStorage.itsPolygons.insert(std::make_pair(areaName, svgString));
      }
      else if (geomType == wkbLineString || geomType == wkbMultiLineString)
      {
#ifdef MYDEBUG
        std::cout << "LINE in SVG format: " << svgString << std::endl;
#endif
        theGeometryStorage.itsLines.insert(std::make_pair(areaName, svgString));
      }
      else if (geomType == wkbPoint)
      {
        const OGRPoint* ogrPoint = reinterpret_cast<const OGRPoint*>(geom);
        theGeometryStorage.itsPoints.insert(
            std::make_pair(areaName, std::make_pair(ogrPoint->getX(), ogrPoint->getY())));
      }
    }

    // Add quotation mark in the beginning and in the end
    for (auto& svg : theGeometryStorage.itsLines)
    {
      if (!svg.second.empty() && svg.second[0] != '"')
      {
        svg.second.insert(0, "\"");
        svg.second.append("\"");
      }
    }
    for (auto& svg : theGeometryStorage.itsPolygons)
    {
      if (!svg.second.empty() && svg.second[0] != '"')
      {
        svg.second.insert(0, "\"");
        svg.second.append("\"");
      }
    }
  }  // namespace Gis
  catch (...)
  {
    throw Spine::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// DYNAMIC MODULE CREATION TOOLS

extern "C" void* engine_class_creator(const char* configfile, void* /* user_data */)
{
  return new SmartMet::Engine::Gis::Engine(configfile);
}

extern "C" const char* engine_name()
{
  return "Gis";
}
// ======================================================================
