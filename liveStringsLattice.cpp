#include "liveStringsLattice.h"

bool LiveStringsLattice::addString(const std::string str){
	if(liveStrings.find(str) != liveStrings.end()){
		liveStrings.insert(str);
		return true;
	}
	return false;
}

bool LiveStringsLattice::remString(const std::string str){
	if(liveStrings.find(str) != liveStrings.end()){
		liveStrings.erase(str);
		return true;
	}
	return false;
}


std::string LiveStringsLattice::str(std::string indent=""){
	ostringstream out;
	out << "[LiveStringsLattice: liveStrings = [\n";
	for(std::set<std::string>::iterator it=liveStrings.begin(); it!=liveStrings.end(); it++) {
		out << *it ;
		out << ";;";
	}
	out << "]]\n";
	return out.str();
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
