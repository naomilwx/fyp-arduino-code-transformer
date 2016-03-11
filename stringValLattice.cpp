#include "stringValLattice.h"

void StringValLattice::initialize() {

}

StringValLattice::StringValLattice(const std::string& str) {
  this->possibleVals.insert(str);
  this->level = StringValLattice::CONSTANT;
}

//StringValLattice::StringValLattice(const char* str): StringValLattice(std::string(str)) {
//}

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

bool StringValLattice::setPossibleVals(std::string val) {
  if(possibleVals.size() == 1 && (possibleVals.find(val) != possibleVals.end())){
	  return false;
  }
  possibleVals.clear();
  possibleVals.insert(val);
  return true;
}

bool StringValLattice::addPossibleVal(const std::string& val) {
	if(possibleVals.find(val) == possibleVals.end()) {
		printf("found string: %s\n",val.c_str());
		possibleVals.insert(val);
		if(level != StringValLattice::TOP && possibleVals.size() == 1) {
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
	return diff;
}

Lattice* StringValLattice::copy() const{
	return new StringValLattice(*this);
}

void StringValLattice::copy(Lattice* lat) {
	StringValLattice* other = dynamic_cast<StringValLattice *>(lat);
	if(this->level != StringValLattice::BOTTOM && other->level == StringValLattice::CONSTANT) {
		if(this->possibleVals != other->possibleVals) {
			this->level = StringValLattice::MODIFIED;
		}
	} else {
		this->level = other->level;
	}
	this->possibleVals = other->possibleVals;
}

std::string StringValLattice::str(std::string indent){
	ostringstream out;
	out << "[StringValLattice: level="<<getLevel() <<" , ";
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
	bool changed = false;
	StringValLattice* other = dynamic_cast<StringValLattice *>(lat);
	if((level == StringValLattice::CONSTANT || level == StringValLattice::MODIFIED) && (other->level == StringValLattice::CONSTANT || other->level == StringValLattice::MODIFIED)) {
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

