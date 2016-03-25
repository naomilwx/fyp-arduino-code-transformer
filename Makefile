ROSE_INSTALL=/home/ROSE/RoseInstallTree
BOOST_INSTALL=/home/ROSE/BoostInstallTree

INSTALL_INCLUDES=-I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

## Arduino Library
ARDUINO=/root/Arduino/hardware
ARDUINO_TOOLS=$(ARDUINO)/tools/avr/avr/include
ARDUINO_VARIANTS=$(ARDUINO)/arduino/avr/variants/standard
ARDUINO_CORE=$(ARDUINO)/arduino/avr/cores/arduino

GPP=g++ -std=c++11
all: analyser

testprop: testStringValPropagation.o ctUtils.o stringValLattice.o ctOverallDataflowAnalyser.o stringValPropagation.o stringLiteralAnalysis.o codeSimplifier.o
	$(GPP) testStringValPropagation.o ctUtils.o ctOverallDataflowAnalyser.o stringValPropagation.o stringValLattice.o stringLiteralAnalysis.o codeSimplifier.o $(INSTALL_INCLUDES) -o testprop
	
analyser: testStringLiteralAnalysis.o stringLiteralAnalysis.o 
	$(GPP) testStringLiteralAnalysis.o stringLiteralAnalysis.o $(INSTALL_INCLUDES) -o analyser

itransform: simplifyingTransformer.o ctUtils.o stringValLattice.o ctOverallDataflowAnalyser.o stringValPropagation.o stringLiteralAnalysis.o codeSimplifier.o
	$(GPP) simplifyingTransformer.o ctUtils.o ctOverallDataflowAnalyser.o stringValPropagation.o stringValLattice.o stringLiteralAnalysis.o codeSimplifier.o $(INSTALL_INCLUDES) -o itransform

ptransform: simpleProgmemTransformer.o ctUtils.o stringValLattice.o ctOverallDataflowAnalyser.o stringValPropagation.o stringLiteralAnalysis.o basicProgmemTransform.o
	$(GPP) simpleProgmemTransformer.o ctUtils.o stringValLattice.o ctOverallDataflowAnalyser.o stringValPropagation.o stringLiteralAnalysis.o basicProgmemTransform.o $(INSTALL_INCLUDES) -o ptransform

simpleProgmemTransformer.o: simpleProgmemTransformer.cpp
	$(GPP) -c simpleProgmemTransformer.cpp $(INSTALL_INCLUDES)

simplifyingTransformer.o: simplifyingTransformer.cpp
	$(GPP) -c simplifyingTransformer.cpp $(INSTALL_INCLUDES)

testStringValPropagation.o: testStringValPropagation.cpp
	$(GPP) -c testStringValPropagation.cpp $(INSTALL_INCLUDES)
	
testStringLiteralAnalysis.o: testStringLiteralAnalysis.cpp
	$(GPP) -c testStringLiteralAnalysis.cpp $(INSTALL_INCLUDES)
	
basicProgmemTransform.o: basicProgmemTransform.cpp basicProgmemTransform.h
	$(GPP) -c basicProgmemTransform.cpp 	$(INSTALL_INCLUDES)

spaceOptimisedTransformer.o: spaceOptimisedTransformer.cpp spaceOptimisedTransformer.h
	$(GPP) -c spaceOptimisedTransformer.cpp $(INSTALL_INCLUDES)
	
stringLiteralAnalysis.o: stringLiteralAnalysis.cpp stringLiteralAnalysis.h
	$(GPP) -c stringLiteralAnalysis.cpp $(INSTALL_INCLUDES)

stringValLattice.o: stringValLattice.cpp stringValLattice.h
	$(GPP) -c stringValLattice.cpp $(INSTALL_INCLUDES)

stringValPropagation.o: stringValPropagation.cpp stringValLattice.h
	$(GPP) -c stringValPropagation.cpp $(INSTALL_INCLUDES)

codeSimplifier.o: codeSimplifier.cpp codeSimplifier.h
	$(GPP) -c codeSimplifier.cpp $(INSTALL_INCLUDES)

ctOverallDataflowAnalyser.o: ctOverallDataflowAnalyser.cpp ctOverallDataflowAnalyser.h
	$(GPP) -c ctOverallDataflowAnalyser.cpp $(INSTALL_INCLUDES)

ctUtils.o: ctUtils.cpp ctUtils.h
	$(GPP) -c ctUtils.cpp $(INSTALL_INCLUDES)
	
check: analyser
	./analyser -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)

checkprop: testprop
	./testprop -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)

intertransform: itransform
	./itransform -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)
	
progmemtransform: ptransform
	./ptransform -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)

combined:
	./itransform -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file); \
	./ptransform -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE)  rose_$(notdir $(file)); \ ;	
	
clean:
	rm *o analyser testprop itransform ptransform
