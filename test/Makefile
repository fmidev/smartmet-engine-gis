PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

REQUIRES = geos gdal configpp

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

CFLAGS = -DUNIX -O0 -g $(FLAGS)

ifneq "$(wildcard /usr/geos38/include)" ""
  INCLUDES += -I/usr/geos38/include
  LIBS += -L/usr/geos38/lib64
else
  INCLUDES += -I/usr/include/geos
endif

INCLUDES += \
	-I../gis \
	-isystem /usr/include/smartmet

LIBS += \
	../gis.so \
	$(REQUIRED_LIBS) \
	-lsmartmet-spine \
	-lboost_date_time \
	-lboost_thread \
	-lboost_filesystem \
	-lboost_regex \
	-lboost_iostreams \
	-lboost_system \
	-lbz2 -lz -lpthread

TEST_PREPARE_TARGETS := cnf/gis.conf
TEST_FINISH_TARGETS := dummy
TEST_DB_DIR := $(shell pwd)/tmp-geonames-db

ifdef CI
GEONAMES_HOST_EDIT := sed -e 's|"smartmet-test"|"$(TEST_DB_DIR)"|g'
TEST_PREPARE_TARGETS += geonames-database start-geonames-db
TEST_FINISH_TARGETS += stop-geonames-db
else
GEONAMES_HOST_EDIT := cat
endif

all: $(PROG)

clean:
	rm -f $(PROG) *~
	rm -f cnf/gis.conf
	-$(MAKE) stop-geonames-db
	rm -rm tmp-geonames-db*

test: $(PROG) $(TEST_PREPARE_TARGETS)
	rm -f failures/*
	if ./EngineTest; then \
		ok=true; $(MAKE) $(TEST_FINISH_TARGETS); \
	else \
		ok=false; $(MAKE) $(TEST_FINISH_TARGETS); \
	fi; $$ok

geonames-database:
	@-$(MAKE) stop-geonames-db
	rm -rf tmp-geonames-db
	if ! /usr/share/smartmet/test/db/create-local-db.sh $(TEST_DB_DIR) >tmp-geonames-db.log 2>&1 ; then \
	    cat tmp-geonames-db.log; \
	    false; \
	fi

start-geonames-db: geonames-database
	/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) start -w

stop-geonames-db:
	-/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) stop

dummy:

cnf/gis.conf:
	$(GEONAMES_HOST_EDIT) $@.in >$@

$(PROG) : % : %.cpp Makefile ../gis.so
	$(CXX) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)

.PHONY: cnf/gis.conf dummy
