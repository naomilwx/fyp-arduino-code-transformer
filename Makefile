SHELL := /bin/bash
ROSE_INSTALL=/home/ROSE/RoseInstallTree
BOOST_INSTALL=/home/ROSE/BoostInstallTree


SRCDIR=src
TARGETDIR=bin
BUILDDIR=build

SRCEXT=cpp
#SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
#OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

INSTALL_INCLUDES=-I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

## Arduino Library
ARDUINO=/root/Arduino
ARDUINO_TOOLS=$(ARDUINO)/hardware/tools/avr/avr/include
ARDUINO_VARIANTS=$(ARDUINO)/hardware/arduino/avr/variants/standard
ARDUINO_CORE=$(ARDUINO)/hardware/arduino/avr/cores/arduino
ARDUINO_LIBRARIES=$(addprefix -I, $(shell find $(ARDUINO)/hardware/arduino/avr/libraries/* -type d -print))
ADDITIONAL_LIBRARIES=$(addprefix -I, $(shell find $(ARDUINO)/libraries/*/src -type d -print)) 
ESP8266=-I/root/esp8266/Arduino/tools/sdk/include $(addprefix -I, $(shell find /root/esp8266/Arduino/libraries/* -type d -print))
ARDUINO_INCLUDES=-I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS) -I$(ARDUINO_CORE) -I$(ARDUINO_LIBRARIES) $(ADDITIONAL_LIBRARIES) $(ESP8266)

GPP=g++ -std=c++11

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(TARGETDIR)
	@echo "Compiling $<..."; $(GPP)  $(INSTALL_INCLUDES) -c -o $@ $<

testpropobjs=$(BUILDDIR)/testStringValPropagation.o $(BUILDDIR)/ctUtils.o $(BUILDDIR)/stringValLattice.o $(BUILDDIR)/ctOverallDataflowAnalyser.o $(BUILDDIR)/stringValPropagation.o $(BUILDDIR)/stringLiteralAnalysis.o
testprop: $(testpropobjs)
	$(GPP) $(testpropobjs) $(INSTALL_INCLUDES) -o $(TARGETDIR)/testprop
	
analyserobjs=$(BUILDDIR)/testStringLiteralAnalysis.o $(BUILDDIR)/stringLiteralAnalysis.o
analyser: $(analyserobjs)
	$(GPP) $(analyserobjs) $(INSTALL_INCLUDES) -o $(TARGETDIR)/analyser

itobjs=$(BUILDDIR)/simplifyingTransformer.o $(BUILDDIR)/ctUtils.o $(BUILDDIR)/stringValLattice.o $(BUILDDIR)/ctOverallDataflowAnalyser.o $(BUILDDIR)/stringValPropagation.o $(BUILDDIR)/stringLiteralAnalysis.o $(BUILDDIR)/codeSimplifier.o

itransform: $(itobjs) 
	$(GPP) $(itobjs) $(INSTALL_INCLUDES) -o $(TARGETDIR)/itransform

ptobjs=$(BUILDDIR)/simpleProgmemTransformer.o $(BUILDDIR)/ctUtils.o $(BUILDDIR)/stringValLattice.o $(BUILDDIR)/ctOverallDataflowAnalyser.o $(BUILDDIR)/stringValPropagation.o $(BUILDDIR)/stringLiteralAnalysis.o $(BUILDDIR)/basicProgmemTransform.o 
ptransform: $(ptobjs)
	$(GPP) $(ptobjs) $(INSTALL_INCLUDES) -o $(TARGETDIR)/ptransform
	
#check: analyser
#	./analyser -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)

checkprop: testprop
	./$(TARGETDIR)/testprop -DROSE -c  -I.-I$(ROSE_INSTALL)/lib $(ARDUINO_INCLUDES) $(file) 

intertransform: itransform
	./$(TARGETDIR)/itransform -DROSE -c  -I. -I$(ROSE_INSTALL)/lib $(ARDUINO_INCLUDES) $(file)
	
progmemtransform: ptransform
	./$(TARGETDIR)/ptransform -DROSE -c  -I. -I$(ROSE_INSTALL)/lib $(ARDUINO_INCLUDES) $(file)

combined: itransform ptransform
	set -e; \
	source ./set.rose ; \
	./$(TARGETDIR)/itransform  -c   $(ARDUINO_INCLUDES) $(userincl)  $(file); \
	./$(TARGETDIR)/ptransform  -c   $(ARDUINO_INCLUDES) $(userincl) rose_$(notdir $(file))

clean:
	rm *o -r $(BUILDDIR) $(TARGETDIR)
