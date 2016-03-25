ROSE_INSTALL=/home/ROSE/RoseInstallTree
BOOST_INSTALL=/home/ROSE/BoostInstallTree

## Arduino Library
ARDUINO=/root/Arduino/hardware
ARDUINO_TOOLS=$(ARDUINO)/tools/avr/avr/include
ARDUINO_VARIANTS=$(ARDUINO)/arduino/avr/variants/standard
ARDUINO_CORE=$(ARDUINO)/arduino/avr/cores/arduino

GPP=g++ -std=c++11
all: analyser

testprop: testStringValPropagation.o ctUtils.o stringValLattice.o ctOverallDataflowAnalyser.o stringValPropagation.o stringLiteralAnalysis.o codeSimplifier.o
	$(GPP) testStringValPropagation.o ctUtils.o ctOverallDataflowAnalyser.o stringValPropagation.o stringValLattice.o stringLiteralAnalysis.o codeSimplifier.o -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system -o testprop
	
analyser: testStringLiteralAnalysis.o stringLiteralAnalysis.o 
	$(GPP) testStringLiteralAnalysis.o stringLiteralAnalysis.o -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system -o analyser

itransform: simplifyingTransformer.o ctUtils.o stringValLattice.o ctOverallDataflowAnalyser.o stringValPropagation.o stringLiteralAnalysis.o codeSimplifier.o
	$(GPP) simplifyingTransformer.o ctUtils.o ctOverallDataflowAnalyser.o stringValPropagation.o stringValLattice.o stringLiteralAnalysis.o codeSimplifier.o -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system -o itransform

ptransform: simpleProgmemTransformer.o ctUtils.o stringValLattice.o ctOverallDataflowAnalyser.o stringValPropagation.o stringLiteralAnalysis.o basicProgmemTransformer.o
	$(GPP) simpleProgmemTransformer.o ctUtils.o stringValLattice.o ctOverallDataflowAnalyser.o stringValPropagation.o stringLiteralAnalysis.o basicProgmemTransformer.o -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system -o ptransform

simpleProgmemTransformer.o: simpleProgmemTransformer.cpp
	$(GPP) -c simpleProgmemTransformer.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

simplifyingTransformer.o: simplifyingTransformer.cpp
	$(GPP) -c simplifyingTransformer.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

testStringValPropagation.o: testStringValPropagation.cpp
	$(GPP) -c testStringValPropagation.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
testStringLiteralAnalysis.o: testStringLiteralAnalysis.cpp
	$(GPP) -c testStringLiteralAnalysis.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
basicProgmemTransformer.o: basicProgmemTransformer.cpp
	$(GPP) -c basicProgmemTransformer.cpp 	-I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

spaceOptimisedTransformer.o: spaceOptimisedTransformer.cpp spaceOptimisedTransformer.h
	$(GPP) -c spaceOptimisedTransformer.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
stringLiteralAnalysis.o: stringLiteralAnalysis.cpp stringLiteralAnalysis.h
	$(GPP) -c stringLiteralAnalysis.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

stringValLattice.o: stringValLattice.cpp stringValLattice.h
	$(GPP) -c stringValLattice.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

stringValPropagation.o: stringValPropagation.cpp stringValLattice.h
	$(GPP) -c stringValPropagation.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

codeSimplifier.o: codeSimplifier.cpp codeSimplifier.h
	$(GPP) -c codeSimplifier.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

ctOverallDataflowAnalyser.o: ctOverallDataflowAnalyser.cpp ctOverallDataflowAnalyser.h
	$(GPP) -c ctOverallDataflowAnalyser.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

ctUtils.o: ctUtils.cpp ctUtils.h
	$(GPP) -c ctUtils.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
check: analyser
	./analyser -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)

checkprop: testprop
	./testprop -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)

intertransform: itransform
	./itransform -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)
	
progmemtransform: ptransform
	./ptransform -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)
clean:
	rm *o analyser testprop itransform ptransform
