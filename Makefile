ROSE_INSTALL=/home/ROSE/RoseInstallTree
BOOST_INSTALL=/home/ROSE/BoostInstallTree

all: analyser

analyser: testStringLiteralAnalysis.o stringLiteralAnalysis.o stringValLattice.o stringValPropagation.o
	g++ testStringLiteralAnalysis.o stringLiteralAnalysis.o stringValLattice.o stringValPropagation.o -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system -o analyser

testStringLiteralAnalysis.o: testStringLiteralAnalysis.cpp
	g++ -c testStringLiteralAnalysis.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
stringLiteralAnalysis.o: stringLiteralAnalysis.cpp
	g++ -c stringLiteralAnalysis.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

stringValLattice.o: stringValLattice.cpp
	g++ -c stringValLattice.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system

stringValPropagation.o: stringValPropagation.cpp
	g++ -c stringValPropagation.cpp -I$(BOOST_INSTALL)/include -I$(ROSE_INSTALL)/include/rose -L$(ROSE_INSTALL)/lib -lrose -L$(BOOST_INSTALL)/lib -lboost_iostreams -lboost_system
	
clean:
	rm *o analyser