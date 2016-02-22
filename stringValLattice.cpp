#include "stringValLattice.h"

void StringValLattice::initialize() {

}

StringValLattice::StringValLattice(std::string str) {
  std::string * s = new std::string(str);
  this->possibleVals.insert(*s);
  this->level = StringValLattice::CONSTANT;
}

StringValLattice::StringValLattice(const char* str): StringValLattice(std::string(str)) {
}

StringValLattice::StringValLattice(const StringValLattice &lat) {
  this->possibleVals = lat.possibleVals;
  this->level = lat.level;
}

StringValLattice::ValLevel StringValLattice::getLevel() const {
  return level;
}

bool StringValLattice::setLevel(ValLevel l) {
  bool diff = (level != l);
  level = l;
  return diff;
}

std::set<std::string> StringValLattice::getPossibleVals() const{
	return possibleVals;
}

bool StringValLattice::setPossibleVals(std::set<std::string> vals) {
  bool diff = (possibleVals != vals);
  possibleVals = vals;
  return diff;
}

bool StringValLattice::addPossibleVal(std::string val) {
	if(possibleVals.find(val) == possibleVals.end()) {
		possibleVals.insert(val);
		if(possibleVals.size() == 1) {
			level = StringValLattice::CONSTANT;
		} else {
			level = StringValLattice::MULTIPLE;
		}
		return true;
	}
	return false;
}

bool StringValLattice::setBottom() {
	bool diff = (level != StringValLattice::BOTTOM);
	level = StringValLattice::BOTTOM;
	possibleVals.clear();
	return level;
}

Lattice* StringValLattice::copy() const{
	return new StringValLattice(*this);
}

void StringValLattice::copy(Lattice* lat) {
	StringValLattice* other = dynamic_cast<StringValLattice *>(lat);
	this->possibleVals = other->possibleVals;
	this->level = other->level;
}

std::string StringValLattice::str(std::string indent){
	ostringstream out;
	out << "[StringValLattice: level="<<getLevel();
	out << indent << "possibleVals=[";
	for(std::set<std::string>::iterator it=possibleVals.begin(); it!=possibleVals.end(); it++) {
		out << *it ;
		out << ";;";
	}
	out << "]]";
	return out.str();
}

bool StringValLattice::operator==(Lattice *lat) {
	StringValLattice* other = dynamic_cast<StringValLattice *>(lat);
	return (level == other->level) && (possibleVals == other->possibleVals);
}

bool StringValLattice::meetUpdate(Lattice *lat){
	bool changed;
	StringValLattice* other = dynamic_cast<StringValLattice *>(lat);
	if(level == StringValLattice::CONSTANT && other->level == StringValLattice::CONSTANT) {
		if(other->possibleVals == possibleVals) {
			changed = false;
		}else {
			changed = true;
			level = StringValLattice::MULTIPLE;
			possibleVals.insert(other->possibleVals.begin(), other->possibleVals.end());
		}
	} else {
		changed = (level < other->level) || possibleVals != other->possibleVals;
		if(possibleVals != other->possibleVals) {
			possibleVals.insert(other->possibleVals.begin(), other->possibleVals.end());
		}
		if(level < other->level) {
			level = other-> level;
		}
	}
	return changed;
}

