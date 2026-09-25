# GIS engine developer guide

This guide is for developers who change `smartmet-engine-gis`, or use it from a plugin.
The engine gives plugins shared access to map geometries in PostGIS (coastlines, borders,
areas, the geometries of named locations), the CRS registry, and metadata of PostGIS
tables, with caches. The geometry and projection code itself is in the
[gis library](https://github.com/fmidev/smartmet-library-gis) (`Fmi::PostGIS`,
`Fmi::OGR`, `Fmi::SpatialReference`, …).

[CLAUDE.md](../CLAUDE.md) has the architecture summary.

## Contents

1. [Building and testing](#1-building-and-testing)
2. [The API](#2-the-api)
3. [Caches](#3-caches)
4. [Configuration](#4-configuration)
5. [Compatibility](#5-compatibility)
6. [Known pitfalls](#6-known-pitfalls)

---

## 1. Building and testing

```bash
make
make test     # tframe tests; need the PostGIS test database (test/cnf/gis.conf.in)
cd test && make EngineTest && ./EngineTest
```

The test boots a `Spine::Reactor` with the local `gis.so` and queries the database. CI
creates a local test database; locally the configuration points at `smartmet-test:5444`.

## 2. The API

| Call | Returns |
|------|---------|
| `getCRSRegistry()` | The shared `Spine::CRSRegistry` (CRS names and EPSG codes → spatial references and their attributes), loaded from `crsDefinitionDir`. Used by the OGC plugins. |
| `getShape(sr, options)` | One geometry: a geometry collection of the selected rows, transformed to `sr` (or left in the data's CRS with `nullptr`), with polygons smaller than `minarea` removed. |
| `getFeatures([sr,] options)` | The selected rows as features, with the requested attribute fields. |
| `getMetaData(options)` | Bounding box and time steps of a PostGIS table, with configured overrides (the WMS plugin's PostGIS layers use it for their capabilities). |
| `populateGeometryStorage(identifiers, storage)` | Loads named geometries (points, lines, polygons) from several PostGIS sources into a `GeometryStorage`, with SVG and OGR forms. The timeseries, edr and textgen plugins use it for named areas and paths that are not geonames places. |
| `getCacheStats()` | Statistics of the three caches. |

`MapOptions` selects the data: `pgname` (which configured connection), `schema`, `table`,
`fieldnames`, an optional `where` clause, and the simplification settings: `minarea`
(drop small polygons), `mindistance`, and a `simplifier` (the gis library's line
simplification), applied to each feature.

`Normalize` normalises names for the geometry storage lookups.

## 3. Caches

| Cache | Key | Content |
|-------|-----|---------|
| Geometry | connection, schema, table, where, CRS WKT, simplification | `getShape()` results |
| Features | the same plus the field names | `getFeatures()` results |
| Envelope | hash of the table and options | Table extents for metadata |

`cache.max_size` sizes them. The caches never expire by time: geometries changed in the
database are seen only after the entries are evicted or the server restarts.

## 4. Configuration

| Key | Meaning |
|-----|---------|
| `postgis` | Default connection (`host`, `port`, `database`, `username`, `password`, `encoding`), and optional named sub-groups for more databases, selected with `MapOptions::pgname`. |
| `crsDefinitionDir` | Directory of CRS definition files (relative to the configuration file allowed). EPSG definitions themselves come from PROJ's `proj.db`. |
| `cache.max_size` | Cache entries. |
| `gdal` | Key-value pairs passed to `CPLSetConfigOption`. |
| `info` | Per schema/table overrides of `bbox` (W, E, S, N) and `timestep` (ISO duration) for `getMetaData()`. |
| `default_epsg` | SRID for geometries that have none. |
| `quiet` | Suppress warnings (default true). |

`GdalUtils.h` compiles different code for different GDAL versions (`GDAL_VERSION_ID`);
keep it working for all GDAL versions the spec file allows.

## 5. Compatibility

The API is non-virtual. `MapOptions`, `MetaDataQueryOptions` and `GeometryStorage` are
built or read by plugins, so changing their layout requires rebuilding the plugins
(wms, timeseries, edr, …) together with the engine.

## 6. Known pitfalls

* **No time-based expiry** (§3): database edits need a restart (or a cache flush) to show.
* **`where` is inserted into SQL as is.** It comes from product and plugin configuration,
  never from request parameters; keep it that way.
* **CRS objects are shared.** Spatial references from the registry or the gis library's
  factory must not be modified in place: concurrent users of a shared
  `OGRSpatialReference` corrupt each other. Clone before changing.
