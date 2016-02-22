ROSE_INSTALL=/home/ROSE/RoseInstallTree
BOOST_INSTALL=/home/ROSE/BoostInstallTree

## Arduino Library
ARDUINO=/root/Arduino/hardware
ARDUINO_TOOLS=$(ARDUINO)/tools/avr/avr/include
ARDUINO_VARIANTS=$(ARDUINO)/arduino/avr/variants/standard
ARDUINO_CORE=$(ARDUINO)/arduino/avr/cores/arduino

GPP=g++ -std=c++11
all: analyser

testprop: testStringValPropagation.o stringValLattice.o stringValPropagation.o stringLiteralAnalysis.o
	$(GPP) testStringValPropagation.o stringValPropagation.o stringValLattice.o stringLiteralAnalysis.o -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system -o testprop
	
analyser: testStringLiteralAnalysis.o stringLiteralAnalysis.o 
	$(GPP) testStringLiteralAnalysis.o stringLiteralAnalysis.o -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system -o analyser

testStringValPropagation.o: testStringValPropagation.cpp
	$(GPP) -c testStringValPropagation.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
testStringLiteralAnalysis.o: testStringLiteralAnalysis.cpp
	$(GPP) -c testStringLiteralAnalysis.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
stringLiteralAnalysis.o: stringLiteralAnalysis.cpp stringLiteralAnalysis.h
	$(GPP) -c stringLiteralAnalysis.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

stringValLattice.o: stringValLattice.cpp stringValLattice.h
	$(GPP) -c stringValLattice.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

stringValPropagation.o: stringValPropagation.cpp stringValLattice.h
	$(GPP) -c stringValPropagation.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

stringLivenessAnalysis.o: stringLivenessAnalysis.cpp stringLivenessAnalysis.h
	$(GPP) -c stringLivenessAnalysis.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
check: analyser
	./analyser -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)

checkprop: testprop
	./testprop -DROSE -c  -I. -I$(ROSE_INSTALL)/lib -I$(ARDUINO_TOOLS) -I$(ARDUINO_VARIANTS)  -I$(ARDUINO_CORE) $(file)
	
clean:
	rm *o analyser *.dot
