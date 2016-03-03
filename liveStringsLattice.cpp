#include "liveStringsLattice.h"

bool LiveStringsLattice::addString(const std::string& str){
	if(liveStrings.find(str) == liveStrings.end()){
		liveStrings.insert(str);
		return true;
	}
	return false;
}

bool LiveStringsLattice::remString(const std::string& str){
	if(liveStrings.find(str) != liveStrings.end()){
		liveStrings.erase(str);
		return true;
	}
	return false;
}

bool LiveStringsLattice::addStringVar(varID var) {
	if(liveStringVars.find(var) == liveStringVars.end()){
		liveStringVars.insert(var);
		return true;
	}
	return false;
}

bool LiveStringsLattice::remStringVar(varID var){
	if(liveStringVars.find(var) != liveStringVars.end()){
		liveStringVars.erase(var);
		return true;
	}
	return false;
}

std::string LiveStringsLattice::str(std::string indent){
	ostringstream out;
	out << "[LiveStringsLattice:";
	if(liveStrings.size() > 0)
		out << "liveStrings = [";
	for(std::set<std::string>::iterator it=liveStrings.begin(); it!=liveStrings.end(); it++) {
		out << *it ;
		out << ";;";
	}
	if(liveStringVars.size() > 0)
		out << "]\n liveStringsVars = [";
	for(auto &var: liveStringVars) {
		out << var;
		out << ";;";
	}
	out << "]\n";
	return out.str();
}

std::set<std::string> LiveStringsLattice::getStrings() const {
	return liveStrings;
}

Lattice* LiveStringsLattice::copy() const{
	return new LiveStringsLattice(*this);
}

void LiveStringsLattice::copy(Lattice* that) {
	LiveStringsLattice *lat = dynamic_cast<LiveStringsLattice*>(that);
	this->liveStrings = lat->liveStrings;
	this->liveStringVars = lat->liveStringVars;
}

bool LiveStringsLattice::meetUpdate(Lattice *other) {
	bool changed = false;
	LiveStringsLattice* lat = dynamic_cast<LiveStringsLattice*>(other);
	for(auto const& item:  lat->liveStrings) {
		if(liveStrings.find(item) == liveStrings.end()) {
			liveStrings.insert(item);
			changed = true;
		}
	}

	for(auto const& item:  lat->liveStringVars) {
		if(liveStringVars.find(item) == liveStringVars.end()) {
			liveStringVars.insert(item);
			changed = true;
		}
	}
	return changed;
}

bool LiveStringsLattice::operator ==(Lattice *lat){
	return liveStrings == dynamic_cast<LiveStringsLattice*>(lat)->liveStrings
			&& liveStringVars == dynamic_cast<LiveStringsLattice*>(lat)->liveStringVars;
}

//LiveStringsFlowLattice

std::string LiveStringsFlowLattice::str(std::string indent){
	ostringstream out;
	out << "[";
	if(flowMap.size() > 0)
		out << "flowMap = [\n";
	for(auto& item: flowMap) {
		out << item.first << ": ";
		out <<  item.second << "\n";
	}
	out << "]\n";
	if(varFlowMap.size() > 0)
		out << "varFlowMap = [\n";
	for(auto& item: varFlowMap) {
		out << item.first << ": ";
		out <<  item.second << "\n";
	}
	out << "]]\n";
	return out.str();
}

Lattice* LiveStringsFlowLattice::copy() const {
	return new LiveStringsFlowLattice(*this);
}

void LiveStringsFlowLattice::copy(Lattice* that) {
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice*>(that);
	this->flowMap = lat->flowMap;
	this->varFlowMap = lat->varFlowMap;
}

bool LiveStringsFlowLattice::meetUpdate(Lattice *other) {
	bool changed = false;
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice*>(other);
	for(const auto& mItem: lat->flowMap){
		FlowVal oflow = mItem.second;
		if(flowMap.find(mItem.first) == flowMap.end()){
			if(oflow == FlowVal::SOURCE){
				flowMap[mItem.first] = FlowVal::AFTER;
			} else {
				flowMap[mItem.first] = oflow;
			}

			changed = true;
		}else {
			FlowVal flow = flowMap[mItem.first];
			if(flow == FlowVal::BEFORE && flow != mItem.second) {
				flowMap[mItem.first] = FlowVal::AFTER;
				changed = true;
			}
		}
	}
	for(const auto& mItem: lat->varFlowMap){
		FlowVal oflow = mItem.second;
		if(varFlowMap.find(mItem.first) == varFlowMap.end()){
			if(oflow == FlowVal::SOURCE){
				varFlowMap[mItem.first] = FlowVal::AFTER;
			} else {
				varFlowMap[mItem.first] = oflow;
			}

			changed = true;
		}else {
			FlowVal flow = varFlowMap[mItem.first];
			if(flow == FlowVal::BEFORE && flow != mItem.second) {
				varFlowMap[mItem.first] = FlowVal::AFTER;
				changed = true;
			}
		}
	}
	return changed;
}

bool LiveStringsFlowLattice::operator ==(Lattice *lat) {
	return flowMap == dynamic_cast<LiveStringsFlowLattice*>(lat)->flowMap;
}

void LiveStringsFlowLattice::setFlowValue(const std::string& str, FlowVal val) {
	flowMap[str] = val;
}

LiveStringsFlowLattice::FlowVal LiveStringsFlowLattice::getFlowValue(const std::string& str) {
	return flowMap[str];
}

void LiveStringsFlowLattice::setFlowValue(varID var, FlowVal val){
	varFlowMap[var] = val;
}

LiveStringsFlowLattice::FlowVal LiveStringsFlowLattice::getFlowValue(varID var){
	return varFlowMap[var];
}

bool LiveStringsFlowLattice::isBeforeStringLiteral(const std::string& str) {
	if(flowMap.find(str) == flowMap.end()) {
		return true;
	}
	return flowMap[str] == FlowVal::BEFORE;
}

bool LiveStringsFlowLattice::isBeforeStringVar(varID var) {
	if(varFlowMap.find(var) == varFlowMap.end()) {
		return true;
	}
	return varFlowMap[var] == FlowVal::BEFORE;
}
