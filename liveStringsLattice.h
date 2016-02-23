#include <string>
#include <set>
#include "latticeFull.h"

class LiveStringsLattice: public FiniteLattice {
protected:
	std::set<std::string> liveStrings;

public:
	LiveStringsLattice(): liveStrings(){}
	LiveStringsLattice(const std::string s): liveStrings(s){}
	LiveStringsLattice(const std::set<std::string> strSet): liveStrings(strSet){}
	LiveStringsLattice(const LiveStringsLattice &lat) {
	  this->liveStrings = lat.liveStrings;
	}
	void initialize() {}

	bool addString(const std::string str);
	bool remString(const std::string str);


	std::string str(std::string indent="");

	Lattice* copy() const;
	void copy(Lattice* that);

	bool meetUpdate(Lattice *other);

	bool operator ==(Lattice *lat);
};
