# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`smartmet-engine-gis` is a SmartMet Server engine that provides shared GIS services (coordinate projections, PostGIS geometry access, spatial reference management) to all server plugins. It is dynamically loaded as `gis.so` by the SmartMet Server daemon.

## Build commands

```bash
make                # Build gis.so
make test           # Run tests (requires PostGIS database; CI sets up a local one)
make format         # Run clang-format on all source files
make clean          # Clean build artifacts
make rpm            # Build RPM package
```

Tests require a running PostGIS database. In CI, a local test database is created automatically. Locally, the test config (`test/cnf/gis.conf.in`) points at `smartmet-test:5444` — you need that database accessible or you need to edit the config.

To run the test binary directly after building:

```bash
cd test && make EngineTest && ./EngineTest
```

## Architecture

The engine has a small, focused API surface:

- **`Engine`** (`gis/Engine.h`) — the main entry point, loaded by the server via the `engine_class_creator` C factory function at the bottom of `Engine.cpp`. Inherits `Spine::SmartMetEngine`. Provides:
  - `getCRSRegistry()` — shared CRS (Coordinate Reference System) definitions for all plugins
  - `getShape()` / `getFeatures()` — fetch and optionally simplify geometries from PostGIS
  - `getMetaData()` — query timesteps and bounding boxes from PostGIS tables
  - `populateGeometryStorage()` — bulk-load named geometries (points, lines, polygons) from multiple PostGIS sources into a `GeometryStorage` for use by plugins like timeseries

- **`Config`** (`gis/Config.h`) — parses the libconfig configuration file. Manages PostGIS connection info (supports multiple named connections), CRS directory loading, GDAL settings, cache sizing, and optional per-table bbox/timestep overrides.

- **`GeometryStorage`** (`gis/GeometryStorage.h`) — in-memory store of named geometries (polygons, lines, points) plus their SVG and OGR representations. Populated by the engine from PostGIS, then queried by plugins.

- **`MapOptions`** / **`MetaData`** — simple structs for query parameters and results.

- **`GdalUtils`** (`gis/GdalUtils.h`) — `GeometryConv` (OGRCoordinateTransformation adapter) and bbox-to-polygon helpers. Contains extensive GDAL version compatibility `#if` blocks.

### Caching

The engine maintains three LRU caches (`Fmi::Cache::Cache`): geometry cache, features cache, and envelope cache. Cache keys incorporate schema, table, field names, WHERE clause, spatial reference WKT, and simplification parameters.

### Key dependencies

- **GDAL/OGR** — geometry I/O, spatial reference handling, coordinate transformations
- **PROJ** — projection definitions (EPSG codes resolved from PROJ's `proj.db`, not from config files)
- **GEOS** — geometry operations
- **`smartmet-library-gis`** (`Fmi::PostGIS`, `Fmi::OGR`, `Fmi::SpatialReference`, etc.) — the heavy lifting for PostGIS reads, OGR operations, and spatial reference factories
- **`smartmet-library-spine`** (`Spine::CRSRegistry`, `Spine::SmartMetEngine`) — server framework and CRS registry

### GDAL version compatibility

`GdalUtils.h` uses `GDAL_VERSION_ID` (computed as `100*MAJOR + MINOR`) to conditionally compile different method signatures. When touching this file, ensure compatibility across the GDAL versions listed in the spec file.

## Configuration

The engine config file (libconfig format) has these sections:
- `crsDefinitionDir` — directory of `.conf` files defining CRS projections (can be relative to the config file)
- `postgis` — default connection info plus optional named sub-groups for multiple databases
- `cache.max_size` — LRU cache entry limit
- `gdal` — key-value pairs passed directly to `CPLSetConfigOption`
- `info` — optional per-schema/table overrides for `bbox` (array of 4 doubles: W, E, S, N) and `timestep` (ISO duration)
- `default_epsg` — fallback SRID for PostGIS geometries with no SRID set
- `quiet` — suppress warnings (default: true)

## Testing

Tests use the `tframe` regression test framework (not Boost.Test). The test boots a `Spine::Reactor` with the engine loaded from the local `gis.so` build, then runs queries against the PostGIS database. Test configs are in `test/cnf/`.
