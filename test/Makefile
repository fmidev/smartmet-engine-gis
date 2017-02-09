PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

MAINFLAGS = -std=c++11 -Wall -W -Wno-unused-parameter

EXTRAFLAGS = -Werror -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast \
	-Wshadow -Woverloaded-virtual

DIFFICULTFLAGS = -Weffc++ -Wunreachable-code -Wredundant-decls

CC = g++
CFLAGS = -DUNIX -O0 -g $(MAINFLAGS) -Wno-write-strings

INCLUDES = \
	-I ../gis \
	-I /usr/include/smartmet \
	`pkg-config --cflags libconfig++`

LIBS = \
	../gis.so \
	-lsmartmet-spine \
	-lsmartmet-gis \
	-lgeos \
	-lgdal \
	-lboost_system \
	`pkg-config --libs libconfig++`

all: $(PROG)
clean:
	rm -f $(PROG) *~

test: $(PROG)
	@echo Running tests:
	@for prog in $(PROG); do \
	./$$prog; \
	done

$(PROG) : % : %.cpp ../gis.so
	$(CC) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)
