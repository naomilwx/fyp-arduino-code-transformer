#ifndef _LIVESTRINGSLATTICE_H_
#define _LIVESTRINGSLATTICE_H_
#include <string>
#include <set>
#include "latticeFull.h"

class LiveStringsLattice: public FiniteLattice {
protected:
	std::set<std::string> liveStrings;
	std::set<varID> liveStringVars;
public:
	LiveStringsLattice(): liveStrings(){}
	LiveStringsLattice(const std::string s) {
		this->liveStrings.insert(s);
	}
	LiveStringsLattice(const std::set<std::string> strSet): liveStrings(strSet){}
	LiveStringsLattice(const LiveStringsLattice &lat) {
	  this->liveStrings = lat.liveStrings;
	  this->liveStringVars = lat.liveStringVars;
	}
	void initialize() {}

	bool addString(const std::string& str);
	bool remString(const std::string& str);

	bool addStringVar(varID var);
	bool remStringVar(varID var);

	std::set<std::string> getStrings() const;
	std::set<varID> getLiveStringVars() const {
		return liveStringVars;
	}

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
	typedef std::map<varID, FlowVal> VarFlowValMap;

	FlowValMap flowMap;
	VarFlowValMap varFlowMap;

	LiveStringsFlowLattice(): flowMap(), varFlowMap(){}
	LiveStringsFlowLattice(LiveStringsFlowLattice *lat){
		this->flowMap = lat->flowMap;
		this->varFlowMap = lat->varFlowMap;
	}

	bool setFlowValue(const std::string& str, FlowVal val);
	FlowVal getFlowValue(const std::string& str);

	bool setFlowValue(varID var, FlowVal val);
	FlowVal getFlowValue(varID);

	bool isBeforeStringLiteral(const std::string& str);
	bool isBeforeStringVar(varID var);

	void initialize() {}

	std::string str(std::string indent="");

	Lattice* copy() const;
	void copy(Lattice* that);

	bool meetUpdate(Lattice *other);

	bool operator ==(Lattice *lat);
};
#endif
