#pragma once
#include <gdal/ogr_spatialref.h>
#include <macgyver/Cache.h>
#include <spine/Thread.h>
#include <list>
#include <memory>

namespace SmartMet
{
namespace Engine
{
namespace Gis
{
class Engine;

class CoordinateTransformationCache
{
 private:
  // The object is stored along with its hash value
  using CacheElement = std::pair<std::size_t, std::unique_ptr<OGRCoordinateTransformation>>;

  // Deleter stores the hash and a reference to cache to which the object is to be returned
  class Deleter
  {
   public:
    explicit Deleter(std::size_t theHash, std::weak_ptr<CoordinateTransformationCache *> theCache);
    void operator()(OGRCoordinateTransformation *ptr);

   private:
    std::size_t itsHash;
    std::weak_ptr<CoordinateTransformationCache *> itsCache;
  };

 public:
  CoordinateTransformationCache();

  using Ptr = std::unique_ptr<OGRCoordinateTransformation, Deleter>;

  // Get transformation object, creating it if necessary using the engine spatial for creating
  // the needed spatial references
  Ptr get(const std::string &theSource, const std::string &theTarget, const Engine &theEngine);

  bool empty() const;
  std::size_t size() const;

 private:
  // Deleter returns object to the cache using this one
  void add(std::size_t hash, std::unique_ptr<OGRCoordinateTransformation> obj);

  mutable Spine::MutexType itsMutex;

  // Destructing this lets weak pointers know the cache is gone
  std::shared_ptr<CoordinateTransformationCache *> itsPointer;

  // First element was used last, last element is oldest
  std::list<CacheElement> itsPool;
};

}  // namespace Gis
}  // namespace Engine
}  // namespace SmartMet

// ======================================================================
