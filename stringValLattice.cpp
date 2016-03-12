#include "stringValLattice.h"
#include <algorithm>

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



// **********************************************************************
//                     PointerAliasLattice
// **********************************************************************
void PointerAliasLattice::initialize(){}
 

// Returns a copy of this lattice
Lattice* PointerAliasLattice::copy() const
{ return new PointerAliasLattice(*this); }


// Copies that lattice into this
void PointerAliasLattice::copy(Lattice* that_arg)
{
    PointerAliasLattice *that = dynamic_cast<PointerAliasLattice*>(that_arg);
    Dbg::dbg<<"Entering COPY : That:" <<that<<" -- "<< that->str(" ") << "This :"<< endl << " -- " << str(" ")<<endl ;
    this->aliasedVariables = that->aliasedVariables;
    this->aliasRelations = that->aliasRelations;
    this->state = that->state;
}

// Checks if that equals this
bool PointerAliasLattice::operator==(Lattice* that_arg)
{
        PointerAliasLattice* that = dynamic_cast<PointerAliasLattice*>(that_arg);
        return aliasedVariables == that->aliasedVariables;
}

//Required function for printing debug information
string PointerAliasLattice::str(string indent)
{
        ostringstream oss;
	oss << "State:" << state <<" ";
        oss<< "Aliases:{ ";
        for(set<varID>::iterator al = aliasedVariables.begin(); al!=aliasedVariables.end(); al++){             
             oss << *al;
             if(al != aliasedVariables.end())
                oss<<"  ";
        }      
        oss<<" }";

        oss << "{";        
        for(set< std::pair<aliasDerefCount, aliasDerefCount> >::iterator alRel = aliasRelations.begin(); alRel!=aliasRelations.end(); alRel++){
            if((alRel->first).var != NULL && (alRel->second).var !=NULL) 
                oss << "(" << isSgVariableSymbol((alRel->first).var)->get_name() 
                    << "," <<(alRel->first).vID
                    << "," << (alRel->first).derefLevel
                    << ") ("
                    << isSgVariableSymbol((alRel->second).var)->get_name() 
                    << "," <<(alRel->second).vID
                    << "," <<(alRel->second).derefLevel 
                    << ")";
            else
                ROSE_ASSERT(((alRel->first).var == NULL && (alRel->second).var ==NULL));
        }
        oss << "}";
        return oss.str();
}


//Add a new Alias
void PointerAliasLattice::setAliasedVariables(varID al)
{
    aliasedVariables.insert(al);
}


//Add a new Alias relation pair
void PointerAliasLattice::setAliasRelation(std::pair < aliasDerefCount, aliasDerefCount > alRel)
{
    aliasRelations.insert(alRel);
}

bool PointerAliasLattice::setState(StateVal state){
    if(this->state != state){
	this->state = state;
	return true;
    }
    return false;
}

//Meet of that lattice with this lattice
/*
    Performs a meet on:
        - that.aliasRelations with this.aliasRelations set. This is a union of sets by ensuring we do not have duplicate elements
        - that.aliasedVariables with this.aliasedVariables set. This can also be viewed as a union of compact representation graphs and is simply a union of sets byt ensuring we do not have duplicates.
    If the union of either of the sets results in modification of this, we set the 'modified' flag to true
*/
bool PointerAliasLattice::meetUpdate(Lattice* that_arg)
{
    bool modified=false;
    PointerAliasLattice *that = dynamic_cast<PointerAliasLattice*>(that_arg);
    Dbg::dbg<<"IN MEETTPDATE That:" << that->str(" ") << "This :"<< str(" ")<<endl ;
    
    if(that->state > state){
	state = that->state;
	modified = true;
    }
    //Union of Aliasrelations
    set< std::pair<aliasDerefCount, aliasDerefCount> > thisAliasRelations= aliasRelations;
    set< std::pair<aliasDerefCount, aliasDerefCount> > thatAliasRelations= that->getAliasRelations();
    for(set< std::pair<aliasDerefCount, aliasDerefCount> >::iterator alRel = thatAliasRelations.begin(); 
        alRel!=thatAliasRelations.end();alRel++ )
    {
       //set::find() doesnt work well on pairs for some reason. Adding a search function for now
       if(!search(thisAliasRelations,*alRel)){
          this->setAliasRelation(*alRel);
          modified = true;
        }
    }

    //Union of aliasedVariables (Compact Representation Graphs)
    set< varID > thisAlias= aliasedVariables;
    set< varID > thatAlias= that->getAliasedVariables();
    Dbg::dbg<<"This alias Size :"<<thisAlias.size() << " That alias Size :"<<thatAlias.size();
    for(set< varID >::iterator al = thatAlias.begin(); al!=thatAlias.end();al++ )
    {
       if(thisAlias.find(*al) == thisAlias.end()){
         this->setAliasedVariables(*al);
         modified = true;
        }
    }
return modified;
}



template <typename T>
bool PointerAliasLattice::search(set<T> thisSet, T value)
{
    for(typename set< T >::iterator item = thisSet.begin(); item!=thisSet.end(); item++ )
    {
        if(*item == value)
          return true;
    }
    return false;
}


//Getter for AliasRelations
set< std::pair<aliasDerefCount, aliasDerefCount> > PointerAliasLattice::getAliasRelations() 
{
    return aliasRelations;
}

//Getter for Aliases
set<varID> PointerAliasLattice::getAliasedVariables()
{
    return aliasedVariables;
}


//Clear Aliases - used for must-aliases
void PointerAliasLattice::clearAliasedVariables()
{
    aliasedVariables.clear();
}
