#pragma once

#include "BBox.h"
#include <string>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
// Some of the information available from EPSG registry at http://www.epsg-registry.org
struct EPSG
{
  BBox bbox;
  std::string name;
  std::string scope;
  int number = 0;
  bool deprecated = false;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
