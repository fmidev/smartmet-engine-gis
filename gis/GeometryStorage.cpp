#include "GeometryStorage.h"
#include "MapOptions.h"
#include "Normalize.h"
#include <macgyver/Exception.h>
#include <spine/HTTP.h>
#include <spine/TableFormatterOptions.h>
#include <spine/TableFormatterFactory.h>
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
    std::string key = name;
    normalize_string(key);

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
    std::string key = name;
    normalize_string(key);

    if (itsPoints.find(key) != itsPoints.end())
      return itsPoints.at(key);
    return std::make_pair(32700.0, 32700.0);
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

    std::string key = name;
    normalize_string(key);

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
    std::string key = name;
    normalize_string(key);

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
    std::string key = name;
    normalize_string(key);
    return itsPolygons.find(key) != itsPolygons.end();
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
    std::string key = name;
    normalize_string(key);
    return itsLines.find(key) != itsLines.end();
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
    std::string key = name;
    normalize_string(key);
    return itsPoints.find(key) != itsPoints.end();
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
      return_list.push_back(item.first);

    for (const auto& item : itsPoints)
      return_list.push_back(item.first);

    return return_list;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::unique_ptr<Spine::Table> GeometryStorage::dumpContents() const
{
  try
  {
    auto table = std::make_unique<Spine::Table>();

    table->setTitle("Geometry Storage Contents");
    table->setNames({"Name", "Type", "Data"});

    int row = 0;
    for (const auto& item : itsPolygons)
    {
      table->set(0, row, item.first);
      table->set(1, row, "Polygon");
      table->set(2, row, item.second);
      row++;
    }

    for (const auto& item : itsLines)
    {
      table->set(0, row, item.first);
      table->set(1, row, "Line");
      table->set(2, row, item.second);
      row++;
    }

    for (const auto& item : itsPoints)
    {
      table->set(0, row, item.first);
      table->set(1, row, "Point");
      std::string point_str = "(" + Fmi::to_string(item.second.first) + ", " + Fmi::to_string(item.second.second) + ")";
      table->set(2, row, point_str);
      row++;
    }

    return table;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void GeometryStorage::dumpContents(std::ostream& out, const std::string& format) const
{
  try
  {
    auto table = dumpContents();
    Spine::TableFormatterOptions options;
    options.setFormatType(format);
    auto formatter = Spine::TableFormatterFactory::create(format);
    out << formatter->format(*table, {}, Spine::HTTP::Request(), options);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
