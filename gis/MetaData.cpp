#include "MetaData.h"
#include <macgyver/Hash.h>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
std::size_t MetaDataQueryOptions::hash_value() const
{
  auto hash = Fmi::hash_value(pgname);
  Fmi::hash_combine(hash, Fmi::hash_value(schema));
  Fmi::hash_combine(hash, Fmi::hash_value(table));
  Fmi::hash_combine(hash, Fmi::hash_value(geometry_column));
  if (time_column)
    Fmi::hash_combine(hash, Fmi::hash_value(*time_column));
  return hash;
}

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
