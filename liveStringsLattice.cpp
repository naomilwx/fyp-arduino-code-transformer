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


std::string LiveStringsLattice::str(std::string indent){
	ostringstream out;
	out << "[LiveStringsLattice: liveStrings = [\n";
	for(std::set<std::string>::iterator it=liveStrings.begin(); it!=liveStrings.end(); it++) {
		out << *it ;
		out << ";;";
	}
	out << "]]\n";
	return out.str();
}

std::set<std::string> LiveStringsLattice::getStrings() const {
	return liveStrings;
}

Lattice* LiveStringsLattice::copy() const{
	return new LiveStringsLattice(*this);
}

void LiveStringsLattice::copy(Lattice* that) {
	this->liveStrings = dynamic_cast<LiveStringsLattice*>(that)->liveStrings;
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
	return changed;
}

bool LiveStringsLattice::operator ==(Lattice *lat){
	return liveStrings == dynamic_cast<LiveStringsLattice*>(lat)->liveStrings;
}

//LiveStringsFlowLattice

std::string LiveStringsFlowLattice::str(std::string indent){
	ostringstream out;
	out << "[flowMap = [\n";
	for(auto& item: flowMap) {
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
	this->flowMap = dynamic_cast<LiveStringsFlowLattice*>(that)->flowMap;
}

bool LiveStringsFlowLattice::meetUpdate(Lattice *other) {
	bool changed = false;
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice*>(other);
	for(const auto& mItem: lat->flowMap){
		FlowVal oflow = mItem.second;
		if(flowMap.find(mItem.first) == flowMap.end()){
			if(mItem.second == FlowVal::SOURCE){
				flowMap[mItem.first] = FlowVal::AFTER;
			} else {
				flowMap[mItem.first] = mItem.second;
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

bool LiveStringsFlowLattice::isBeforeStringLiteral(const std::string& str) {
	if(flowMap.find(str) == flowMap.end()) {
		return true;
	}
	return flowMap[str] == FlowVal::BEFORE;
}
