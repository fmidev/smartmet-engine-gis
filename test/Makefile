PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

REQUIRES = geos gdal configpp

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

CFLAGS = -DUNIX -O0 -g $(FLAGS)

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

all: $(PROG)
clean:
	rm -f $(PROG) *~

test: $(PROG)
	@echo Running tests:
	@for prog in $(PROG); do ./$$prog; done

$(PROG) : % : %.cpp ../gis.so
	$(CXX) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)
