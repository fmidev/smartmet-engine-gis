# smartmet-engine-gis — Feature List

A structured inventory of capabilities provided by the GIS engine.
Use as a checklist when drafting release notes. When new functionality
is added, append the new entry under the matching section (and bump
the *Last updated* line at the bottom).

`smartmet-engine-gis` (output: `gis.so`) is the SmartMet Server engine
that exposes shared GIS services to every plugin — coordinate
reference systems, PostGIS geometry access, and geometry-storage
populated from spatial databases. Plugins call into this engine via
`Spine::Reactor::getEngine<Engine>()`.

---

## 1. Plugin-facing engine API

- **`Engine::getCRSRegistry()`** — shared
  `Spine::CRSRegistry` for all plugins; CRS definitions loaded once,
  used everywhere.
- **`Engine::getShape(SR, options)`** — fetch a single named geometry
  from PostGIS, optionally reprojected to the requested spatial
  reference and simplified per `MapOptions`.
- **`Engine::getFeatures(options)`** — fetch a feature collection
  (multiple geometries with attributes) from PostGIS.
- **`Engine::getFeatures(SR, options)`** — same, reprojected.
- **`Engine::getMetaData(query)`** — bbox + timestep + dimensions
  for a PostGIS table.
- **`Engine::populateGeometryStorage(identifiers, storage)`** —
  bulk-load named geometries (polygons, lines, points) from one or
  more PostGIS sources into a `GeometryStorage` object that other
  plugins (e.g. timeseries) can query.

## 2. PostGIS access

- **Multiple named connections** — the config can declare a default
  PostGIS connection plus any number of named sub-connections.
- **Schema / table / field selection** — every query is parameterised
  by schema + table + geometry field.
- **WHERE clause support** — additional `where` filter per request.
- **Default SRID fallback** — `default_epsg` config key supplies an
  SRID for geometries stored without one.
- **`smartmet-library-gis`-backed reads** — uses `Fmi::PostGIS` for
  the actual SQL plumbing.

## 3. Coordinate / projection system

- **`Spine::CRSRegistry`** — single registry serving every plugin so
  CRS definitions are read once and shared.
- **`crsDefinitionDir`** — directory of per-CRS `.conf` files;
  resolvable relative to the engine's config file.
- **PROJ-backed EPSG resolution** — EPSG codes are looked up against
  PROJ's `proj.db`; not duplicated in config files.
- **GDAL/OGR coordinate transformations** — `GdalUtils::GeometryConv`
  wraps `OGRCoordinateTransformation` with a clean RAII interface.
- **GDAL version compatibility** — `GdalUtils.h` carries `#if`
  guards keyed off `GDAL_VERSION_ID` (computed as
  `100*MAJOR + MINOR`).

## 4. Geometry simplification & normalisation

- **`MapOptions` simplification knobs** — caller-controlled
  tolerance applied before returning a geometry.
- **`Normalize`** — geometry-normalisation helpers used during
  ingestion (e.g. snap-rounding, fixup of self-intersections).
- **Reprojection-aware** — simplification happens in the requested
  output CRS, not the source CRS.

## 5. Metadata API

- **`MetaData`** — per-table metadata: temporal extent, valid times,
  bounding box.
- **`MetaDataQueryOptions`** — filter / scope inputs.
- **Per-table overrides** — config `info` block lets you fix a
  table's `bbox` (`W, E, S, N`) and `timestep` (ISO duration) when
  the database can't supply them.

## 6. Geometry storage

`GeometryStorage`:

- **Named geometry sets** — keyed look-up of polygons, lines, and
  points by name (e.g. "Finland", "Coast").
- **Three representations cached per geometry** — OGR object, SVG
  string, lat/lon points — so any consumer gets the format it needs
  without repeated conversion.
- **Bulk population** — `populateGeometryStorage` accepts a vector
  of `PostGISIdentifier` records, each pointing at a schema / table /
  geometry-name in PostGIS.

## 7. Caching

Three LRU caches inside the engine (each `Fmi::Cache::Cache`):

- **Geometry cache** — single-shape results keyed by schema + table
  + field + WHERE + spatial reference WKT + simplification.
- **Features cache** — feature-collection results keyed similarly.
- **Envelope cache** — bounding-box results.
- **`cache.max_size`** — per-cache entry limit set from the config.
- **Cache stats** — exposed through `Fmi::Cache::CacheStats`.

## 8. Configuration

libconfig file with SmartMet extensions:

- **`crsDefinitionDir`** — directory of CRS `.conf` files.
- **`postgis`** — default connection block plus optional named
  sub-connections.
- **`cache.max_size`** — LRU cache entry limit.
- **`gdal`** — key/value pairs passed straight to
  `CPLSetConfigOption`.
- **`info`** — per-schema/table overrides for `bbox` and `timestep`.
- **`default_epsg`** — fallback SRID.
- **`quiet`** — suppress warnings (default `true`).

## 9. Engine factory and lifecycle

- **`engine_class_creator`** C factory function exposed at the
  bottom of `Engine.cpp` for `dlopen`-based loading.
- **Inherits `Spine::SmartMetEngine`** — standard `init()` /
  `shutdown()` hooks.

## 10. Testing

- **`tframe`** regression test framework (FMI's own, not Boost.Test).
- **`test/EngineTest.cpp`** — boots a `Spine::Reactor` with the
  locally-built `gis.so`, runs queries against a PostGIS database.
- **`test/cnf/gis.conf.in`** — test config template (defaults to
  `smartmet-test:5444`; CI creates a local PostGIS instance).
- **Direct test run**:
  `cd test && make EngineTest && ./EngineTest`.

## 11. Documentation & spec

- **`README.md`** — overview.
- **`Doxyfile`** — Doxygen configuration.
- **`smartmet-engine-gis.spec`** — RPM packaging.

## 12. Build & integration

- **Output**: `gis.so`.
- **Loaded at**: `$(prefix)/share/smartmet/engines/gis.so`.
- **Build**: `make` (and `Makefile.clang` for clang).
- **Format**: `make format` runs clang-format.
- **Install**: `make install`.
- **RPM**: `make rpm`.
- **Linked libraries**:
  - **`smartmet-library-gis`** — the heavy lifting (`Fmi::PostGIS`,
    `Fmi::OGR`, `Fmi::SpatialReference`).
  - **`smartmet-library-spine`** — `Spine::CRSRegistry`,
    `Spine::SmartMetEngine`.
- **External libraries**: GDAL/OGR, PROJ, GEOS, libpq, libconfig.
- **CI**: CircleCI on RHEL 8 / RHEL 10 with
  `fmidev/smartmet-cibase-{8,10}` Docker images and the standard
  `ci-build` workflow. CI spins up a local PostGIS for the tests.

---

*Last updated: 2026-06-01.*
