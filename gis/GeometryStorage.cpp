#include "GeometryStorage.h"
#include "MapOptions.h"
#include <macgyver/Exception.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
std::string GeometryStorage::getSVGPath(const std::string& name) const
{
  try
  {
    std::string key = boost::algorithm::to_lower_copy(name);

    if (itsPolygons.find(key) != itsPolygons.end())
      return itsPolygons.at(key);
    else if (itsLines.find(key) != itsLines.end())
      return itsLines.at(key);
    else
      return "";
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::pair<double, double> GeometryStorage::getPoint(const std::string& name) const
{
  try
  {
    std::string key = boost::algorithm::to_lower_copy(name);

    if (itsPoints.find(key) != itsPoints.end())
      return itsPoints.at(key);
    else
      return make_pair(32700.0, 32700.0);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const OGRGeometry* GeometryStorage::getOGRGeometry(const std::string& name, int type) const
{
  try
  {
    OGRGeometry* ret = nullptr;

    std::string key = boost::algorithm::to_lower_copy(name);

    auto geomtype = itsGeometries.find(type);

    if (geomtype != itsGeometries.end())
    {
      const auto& name_to_geom = geomtype->second;

      const auto geom = name_to_geom.find(key);

      if (geom != name_to_geom.end())
      {
        auto g = geom->second;
        ret = g.get();
      }
    }

    return ret;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool GeometryStorage::geoObjectExists(const std::string& name) const
{
  try
  {
    std::string key = boost::algorithm::to_lower_copy(name);

    return (isPolygon(key) || isLine(key) || isPoint(key));
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool GeometryStorage::isPolygon(const std::string& name) const
{
  try
  {
    return itsPolygons.find(boost::algorithm::to_lower_copy(name)) != itsPolygons.end();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool GeometryStorage::isLine(const std::string& name) const
{
  try
  {
    return itsLines.find(boost::algorithm::to_lower_copy(name)) != itsLines.end();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool GeometryStorage::isPoint(const std::string& name) const
{
  try
  {
    return itsPoints.find(boost::algorithm::to_lower_copy(name)) != itsPoints.end();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::list<string> GeometryStorage::areaNames() const
{
  try
  {
    std::list<string> return_list;
    for (const auto& item : itsPolygons)
    {
      return_list.push_back(item.first);
    }
    for (const auto& item : itsPoints)
    {
      return_list.push_back(item.first);
    }

    return return_list;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
