#pragma once

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
struct BBox
{
  BBox(double w, double e, double s, double n) : west(w), east(e), south(s), north(n) {}
  double west;
  double east;
  double south;
  double north;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet
