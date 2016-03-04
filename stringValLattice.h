#ifndef _STRINGVALLATTICE_H_
#define _STRINGVALLATTICE_H_
#include <string>
#include <set>
#include "latticeFull.h"

class StringValLattice: public FiniteLattice {
  public:
	enum ValLevel {
	  BOTTOM,
	  CONSTANT,
	  MULTIPLE,
	  TOP
	};
  protected:
	ValLevel level;
	std::set<std::string> possibleVals;
  
  public:
	StringValLattice(): level(BOTTOM), possibleVals() {}
	StringValLattice(const std::string& str);
	StringValLattice(const StringValLattice &lat);

	ValLevel getLevel() const;
	bool setLevel(ValLevel l);

	std::set<std::string> getPossibleVals() const;
	bool setPossibleVals(std::set<std::string> vals);
	bool addPossibleVal(const std::string& val);
	
	bool setBottom();

	// **********************************************
	// Required definition of pure virtual functions.
	// **********************************************
	void initialize();
	std::string str(std::string indent="");

	Lattice* copy() const;
	void copy(Lattice* that);

	bool meetUpdate(Lattice *other);
	
	bool operator ==(Lattice *lat);
};
#endif
