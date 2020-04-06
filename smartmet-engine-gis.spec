%define DIRNAME gis
%define LIBNAME smartmet-%{DIRNAME}
%define SPECNAME smartmet-engine-%{DIRNAME}
Summary: SmartMet GIS engine
Name: %{SPECNAME}
Version: 20.4.6
Release: 1%{?dist}.fmi
License: MIT
Group: SmartMet/Engines
URL: https://github.com/fmidev/smartmet-engine-gis
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: boost-devel
BuildRequires: gdal30-devel
BuildRequires: geos38-devel
BuildRequires: libconfig-devel
BuildRequires: bzip2-devel
BuildRequires: zlib-devel
BuildRequires: smartmet-library-newbase-devel >= 20.4.6
BuildRequires: smartmet-library-spine-devel >= 20.3.9 
BuildRequires: smartmet-library-gis-devel >= 20.4.6
Requires: gdal30
Requires: geos38
Requires: libconfig
Requires: smartmet-library-spine >= 20.3.9 
Requires: smartmet-library-gis >= 20.4.6
Requires: boost-regex
Requires: boost-date-time
Requires: boost-filesystem
Requires: boost-iostreams
Requires: boost-system
Requires: boost-thread
Provides: %{SPECNAME}
Obsoletes: smartmet-brainstorm-gis < 16.11.1
Obsoletes: smartmet-brainstorm-gis-debuginfo < 16.11.1

%description
FMI SmartMet gis engine

%package -n %{SPECNAME}-devel
Summary: SmartMet %{SPECNAME} development headers
Group: SmartMet/Development
Provides: %{SPECNAME}-devel
Obsoletes: smartmet-brainstorm-gis-devel < 16.11.1
%description -n %{SPECNAME}-devel
SmartMet %{SPECNAME} development headers.

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{SPECNAME}
 
%build -q -n %{SPECNAME}
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files -n %{SPECNAME}
%defattr(0775,root,root,0775)
%{_datadir}/smartmet/engines/%{DIRNAME}.so

%files -n %{SPECNAME}-devel
%defattr(0664,root,root,0775)
%{_includedir}/smartmet/engines/%{DIRNAME}/*.h

%changelog
* Mon Apr  6 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.6-1.fmi
- Start using Fmi::CoordinateTransformation and Fmi::SpatialReference

* Mon Mar 30 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.3.30-1.fmi
- Full repackaging of the server

* Tue Feb 25 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.25-1.fmi
- Use GDAL forward declarations in headers to avoid dependency escalation

* Tue Feb 18 2020 Anssi Reponen <anssi.reponen@fmi.fi> - 20.2.18-2.fmi
- Fix handling of duplicate geometries and geometries with no name (BRAINSTORM-1757)
- Convert geometry SRS always to EPSG:4326
- To merge MULTLINESTRING geometries (roads) use addGeometryDirectly-function
instead of Union-function

* Thu Feb 13 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.13-1.fmi
- Hide GDAL dependencies from headers to reduce dependency escalation

* Fri Feb  7 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.7-1.fmi
- Fixed GDAL dependencies

* Thu Jan 23 2020 Anssi Reponen <anssi.reponen@fmi.fi> - 20.1.23-2.fmi
- Redundant fields removed from postgis_identifier structure (BRAINSTORM-1746)

* Tue Dec 10 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.12.10-1.fmi
- Upgrade to GDAL 3.0 and GEOS 3.8

* Wed Nov 20 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.20-1.fmi
- Rebuilt due to newbase API changes

* Wed Oct 30 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.31-1.fmi
- Rebuilt due to newbase API/ABI changes

* Thu Sep 26 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.26-1.fmi
- Added support for GDAL 2

* Fri Sep 13 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.13-1.fmi
- Never throw in constructors

* Wed May 29 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.5.29-1.fmi
- Added possibility to specify a fixed timestep for database layers

* Mon Oct  1 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.10.1-1.fmi
- Fixed code not to generate too many quotation marks around SVG paths

* Wed Sep 19 2018 Anssi Reponen <anssi.reponen@fmi.fi> - 18.9.19-2.fmi
- Removed duplicate code: start using Fmi::OGR::exportToSvg-function

* Wed Sep 19 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.19-1.fmi
- Added possibility to fix PostGIS table spatial extents in the configuration file

* Mon Sep 17 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.17-1.fmi
- Repackaged due to gis-library API changes

* Fri Sep  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.7-1.fmi
- Updated to EPSG 9.5 data
- Store also EPSG name, scope, source and deprecation status, already stored bbox
- Deprecated configuration variable bbox, added variable epsg

* Wed Jul 25 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.7.25-1.fmi
- Prefer nullptr over NULL

* Mon Jul 23 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.7.23-1.fmi
- Silenced CodeChecker warnings

* Sat Apr  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.7-1.fmi
- Upgrade to boost 1.66

* Tue Mar 20 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.3.20-1.fmi
- Full repackaging of the server

* Wed Mar 7 2018 Anssi Reponen <anssi.reponen@fmi.fi> - 18.3.7-1.fmi
- Use UTC timezone when reading data from PostGIS database (BRAINSTORM-1049)

* Mon Mar  5 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.3.5-1.fmi
- Cache envelope queries to increase WMS GetCapabilities speed

* Tue Feb 20 2018 Anssi Reponen <anssi.reponen@fmi.fi> - 18.2.20-1.fmi
- Checking NULL value in metadata query

* Fri Feb 9 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.2.9-1.fmi
- Repackaged since base class SmartMetEngine size changed

* Wed Jan 31 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.1.31-2.fmi
- Added setting 'default_epsg' to set SRID for geometries whose SRID is not set
- Added setting 'quiet' to disable warnings from unset SRIDs if default_epgs is used

* Wed Jan 31 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.1.31-1.fmi
- Use ST_Extent since not all tables have been analyzed as required by ST_EstimatedExtent

* Thu Nov 30 2017 Anssi Reponen <anssi.reponen@fmi.fi> - 17.11.30-1.fmi
- Function and a class added to get geometries from several PostGIS sources at once (different dbs, tables)

* Wed Nov  1 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.11.1-1.fmi
- Rebuilt due to GIS library API change

* Mon Aug 28 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

* Tue Apr 11 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.4.11-1.fmi
- crsDefinitionDir can now be relative to the configuration file itself

* Sat Apr  8 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.4.8-1.fmi
- Simplified exception handling

* Wed Mar 15 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.3.15-1.fmi
- Recompiled since Spine::Exception changed

* Tue Mar 14 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.3.14-1.fmi
- Switched to use macgyver StringConversion tools

* Wed Jan  4 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.4-1.fmi
- Changed to use renamed SmartMet base libraries

* Wed Nov 30 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.30-1.fmi
- Using test database in sample and test configuration
- Dedicated engine conf for tests (problem with relative conf paths)
- No installation for configuration

* Tue Nov 29 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.29-1.fmi
- Added Three-dimensional Compound Coordinate Reference Systems EPSG::7409 and EPSG::3903

* Tue Nov  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.1-1.fmi
- Namespace changed

* Tue Sep  6 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.9.6-1.fmi
- New exception handler

* Mon Aug 15 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.8.15-1.fmi
- Full recompile

* Wed Jun 29 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.29-1.fmi
- Added 3395 projection for Sentinel Satellite

* Mon Jun 20 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.20-1.fmi
- Made GDAL object allocations safe with smart pointers (BRAINSTORM-694)

* Fri Jun 17 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.17-1.fmi
- Fixed a memory leak in not deleting OGR features

* Tue Jun 14 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.14-1.fmi
- Full recompile

* Thu Jun  2 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.2-1.fmi
- Full recompile

* Wed Jun  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.1-1.fmi
- Added graceful shutdown

* Tue May 17 2016 Tuomo Lauri <tuomo.lauri@fmi.fi> - 16.5.17-2.fmi
- Fixed default pgname. It should probably be mandatory parameter

* Tue May 17 2016 Tuomo Lauri <tuomo.lauri@fmi.fi> - 16.5.17-1.fmi
- Fixed EPSG metadata retrieval

* Mon Apr 18 2016 Tuomo Lauri <tuomo.lauri@fmi.fi> - 16.4.18-1.fmi
- A more efficient bounding box fetch

* Fri Apr  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.4.1-1.fmi
- Simplify geographies using the approximation 1 degree = 110 kilometers

* Tue Mar 22 2016  Santeri Oksman <santeri.oksman@fmi.fi> - 16.3.22-1.fmi
- Moved icemap database to read only side of popper due the performance reasons.

* Fri Mar  4 2016 Tuomo Lauri <tuomo.lauri@fmi.fi> - 16.3.4-1.fmi
- PostGIS metadata query, coordinate grid, fontsize handling, case insensitive comparison corrected

* Tue Feb  2 2016 Tuomo Lauri <tuomo.lauri@fmi.fi> - 16.2.2-1.fmi
- Solving EPSG codes defined with opengis.net notation.

* Mon Jan 18 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.18-1.fmi
- newbase API changed, full recompile

* Mon Jan 11 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.11-1.fmi
- Fixed the code not to cache features which simplify to empty geometries

* Mon Dec 14 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.12.14-1.fmi
- Time column is no longer mandatory in metadata query

* Thu Dec  3 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.12.3-1.fmi
- Added functionality to get Features (shapes with attributes)

* Tue Nov 10 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.11.10-1.fmi
- Avoid string streams to avoid global std::locale locks

* Mon Nov  9 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.11.9-1.fmi
- Using fast case conversion without locale locks when possible

* Mon Oct 26 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.10.26-1.fmi
- Added proper debuginfo packaging
- Added support for icemaps from popper

* Thu Aug 20 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.8.20-1.fmi
- Added support for several source databases

* Tue Aug 18 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.18-1.fmi
- Recompile forced by HTTP API changes

* Mon Aug 17 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.17-1.fmi
- Use -fno-omit-frame-pointer to improve perf use

* Tue Aug  4 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.8.4-1.fmi
- Now using popper-gis database (not open_data_gis)

* Thu Apr  9 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.9-1.fmi
- newbase API changed

* Wed Apr  8 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.8-1.fmi
- Dynamic linking of smartmet libraries into use

* Tue Feb 24 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.24-1.fmi
- Recompiled due to changes in newbase linkage

* Mon Feb 23 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.23-1.fmi
- Fixed caching of simplified geometries

* Mon Jan 26 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.1.26-1.fmi
- Removed default CRS - feature from CRSRegistry as unnecessary. 

* Wed Dec 17 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.12.17-1.fmi
- Recompiled due to spine API changes
- Fixed an invalid epsg 7423 projection configuration.

* Thu Nov 13 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.11.13-1.fmi
- Recompiled due to newbase API changes
- Fixed a null pointer dereference if area simplification removes the entire shape

* Wed Aug 20 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.8.20-1.fmi
- Recompile due to memory leak fix in gis-library

* Fri Aug 15 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.15-1.fmi
- Fixed a memory leak caused by calling delete instead of OGRFree

* Fri Aug  8 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.8-2.fmi
- Added caching of the requested shapes

* Fri Aug  8 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.8-1.fmi
- Added a MapOptions struct to simplify the API

* Thu Aug  7 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.7-2.fmi
- Geometry simplification tolerance is now in kilometers instead of meters

* Thu Aug  7 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.7-1.fmi
- Recompiled with the latest GIS library API supporting geometry simplification
- Added support for simplifying the given geometry

* Wed Aug  6 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.8.6-1.fmi
- Recompiled with the latest newbase to get fixed WKT methods

* Fri Jul 25 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.7.25-1.fmi
- Recompiled with the latest GDAL
- Axis labels support for CRSRegistry.

* Wed Jun 11 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 
- Recompiled with latest libgis

* Wed Jun  4 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.6.4-1.fmi
- First beta release

* Fri May  2 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.2-1.fmi
- Created from parts of the WFS plugin

