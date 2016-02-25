#include <string>
#include <set>
#include "latticeFull.h"

class LiveStringsLattice: public FiniteLattice {
protected:
	std::set<std::string> liveStrings;

public:
	LiveStringsLattice(): liveStrings(){}
	LiveStringsLattice(const std::string s) {
		this->liveStrings.insert(s);
	}
	LiveStringsLattice(const std::set<std::string> strSet): liveStrings(strSet){}
	LiveStringsLattice(const LiveStringsLattice &lat) {
	  this->liveStrings = lat.liveStrings;
	}
	void initialize() {}

	bool addString(const std::string& str);
	bool remString(const std::string& str);

	std::set<std::string> getStrings() const;

	std::string str(std::string indent="");

	Lattice* copy() const;
	void copy(Lattice* that);

	bool meetUpdate(Lattice *other);

	bool operator ==(Lattice *lat);
};

class LiveStringsFlowLattice : public FiniteLattice {

public:
	enum FlowVal {
		  BEFORE,
		  SOURCE,
		  AFTER
		};
	typedef std::map<std::string, FlowVal> FlowValMap;

	FlowValMap flowMap;


	LiveStringsFlowLattice(): flowMap(){}
	LiveStringsFlowLattice(LiveStringsFlowLattice *lat){
		this->flowMap = lat->flowMap;
	}

	void setFlowValue(const std::string& str, FlowVal val);
	FlowVal getFlowValue(const std::string& str);

	bool isBeforeStringLiteral(const std::string& str);

	void initialize() {}

	std::string str(std::string indent="");

	Lattice* copy() const;
	void copy(Lattice* that);

	bool meetUpdate(Lattice *other);

	bool operator ==(Lattice *lat);
};
