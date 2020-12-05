#include "GeometryStorage.h"
#include "MapOptions.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <macgyver/Exception.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

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
    if (itsLines.find(key) != itsLines.end())
      return itsLines.at(key);
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
    return std::make_pair(32700.0, 32700.0);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

const OGRGeometry* GeometryStorage::getOGRGeometry(const std::string& name,
                                                   OGRwkbGeometryType type) const
{
  try
  {
    OGRGeometry* ret = nullptr;

    std::string key = boost::algorithm::to_lower_copy(name);

    if (itsGeometries.find(type) != itsGeometries.end())
    {
      const NameOGRGeometryMap& nameOGRGeometryMap = itsGeometries.find(type)->second;

      if (nameOGRGeometryMap.find(key) != nameOGRGeometryMap.end())
      {
        boost::shared_ptr<OGRGeometry> g = nameOGRGeometryMap.find(key)->second;
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

std::list<std::string> GeometryStorage::areaNames() const
{
  try
  {
    std::list<std::string> return_list;
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
