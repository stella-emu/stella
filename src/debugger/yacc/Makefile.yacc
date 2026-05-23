
### Note: this Makefile is not used in building the main Stella binary!
### Run it here to regenerate stella.tab.cxx and stella.tab.hxx after
### editing stella.y.  Requires Bison >= 3.2.

all: stella.y
	bison stella.y
	@for f in stella.tab.hxx stella.tab.cxx; do \
	  { printf '// NOLINTBEGIN\n'; cat $$f; printf '// NOLINTEND\n'; } > $$f.tmp && mv $$f.tmp $$f; \
	done

calctest: stella.y calctest.cxx YaccParser.cxx YaccParser.hxx
	bison stella.y
	g++ -DPRINT -I.. -I../.. -std=c++20 -O2 -o calctest calctest.cxx YaccParser.cxx stella.tab.cxx
