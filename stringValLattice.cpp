#include "stringValLattice.h"
#include <algorithm>

// **********************************************************************
//                     PointerAliasLattice
// **********************************************************************
void PointerAliasLattice::initialize(){}
 

// Returns a copy of this lattice
Lattice* PointerAliasLattice::copy() const{
	return new PointerAliasLattice(*this);
}


// Copies that lattice into this
void PointerAliasLattice::copy(Lattice* that_arg)
{
    PointerAliasLattice *that = dynamic_cast<PointerAliasLattice*>(that_arg);
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

void PointerAliasLattice::setAliasedVariables(std::set<varID> als){
	for(auto &al: als)
		aliasedVariables.insert(al);
}

//Add a new Alias relation pair
bool PointerAliasLattice::setAliasRelation(std::pair < aliasDerefCount, aliasDerefCount > alRel)
{
    if(aliasRelations.find(alRel) != aliasRelations.end()) {
    	return false;
    }
	aliasRelations.insert(alRel);
	return true;
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

    //Update state
    if(state < StateVal::REASSIGNED_UNKNOWN && aliasedVariables.size() != 1) {
    	state = StateVal::REASSIGNED_UNKNOWN;
    }
    if(that->state > state){
    	state = that->state;
    	modified = true;
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

/**
 * Modified version of FiniteVarsProductLattice
 * */

// Initial blank ctVarsExprsProductLattice
ctVarsExprsProductLattice::ctVarsExprsProductLattice(const DataflowNode& n, const NodeState& state) :
		FiniteVarsExprsProductLattice(n, state), VarsExprsProductLattice(n, state,filter)
{}

// Retrns a blank instance of a VarsExprsProductLattice that only has the fields n and state set
VarsExprsProductLattice* ctVarsExprsProductLattice::blankVEPL(const DataflowNode& n, const NodeState& state)
{
        return new ctVarsExprsProductLattice(n, state);
}


ctVarsExprsProductLattice::ctVarsExprsProductLattice(
                                      Lattice* perVarLattice,
                                      const map<varID, Lattice*>& constVarLattices,
                                      Lattice* allVarLattice,
                                      LiveDeadVarsAnalysis* ldva,
                                      const DataflowNode& n, const NodeState& state, std::map<varID, Lattice *> globalLattices) :
	FiniteVarsExprsProductLattice(perVarLattice, constVarLattices, allVarLattice, ldva, n, state),
    VarsExprsProductLattice(perVarLattice, constVarLattices, allVarLattice, ldva, n, state),
    FiniteProductLattice()
{
		incorporateVarsMap(globalLattices, true);
		verifyFinite();
}

ctVarsExprsProductLattice::ctVarsExprsProductLattice(const ctVarsExprsProductLattice& that) :
		FiniteVarsExprsProductLattice(that), VarsExprsProductLattice(that), FiniteProductLattice()
{
        //Dbg::dbg << "ctVarsExprsProductLattice::copy n="<<n.getNode()<<" = <"<<Dbg::escape(n.getNode()->unparseToString())<<" | "<<n.getNode()->class_name()<<" | "<<n.getIndex()<<">"<<endl;
        verifyFinite();
}

// returns a copy of this lattice
Lattice* ctVarsExprsProductLattice::copy() const {
    return new ctVarsExprsProductLattice(*this);
}

Lattice* ctVarsExprsProductLattice::addSlotForVariable(varID var) {
	if(varLatticeIndex.find(var) == varLatticeIndex.end()) {
		varLatticeIndex[var] = lattices.size();
		Lattice *lat = perVarLattice->copy();
	    lattices.push_back(lat);
	    return lat;
	}
	return lattices[varLatticeIndex[var]];
}

void ctVarsExprsProductLattice::incorporateVarsMap(std::map<varID, Lattice *> lats, bool overwrite) {

	for(auto&item : lats) {
		if(varLatticeIndex.find(item.first) == varLatticeIndex.end()){
			varLatticeIndex[item.first] = lattices.size();
			lattices.push_back(item.second);
		} else if(overwrite){
			lattices[varLatticeIndex[item.first]]->copy(item.second);
		} else {
			Lattice *origLat = lattices[varLatticeIndex[item.first]];
			origLat->meetUpdate(item.second);
		}
	}
}

void ctVarsExprsProductLattice::incorporateVars(Lattice *that_arg, bool(*f)(Lattice*)){
	initialize();

	ctVarsExprsProductLattice* that = dynamic_cast<ctVarsExprsProductLattice*>(that_arg); ROSE_ASSERT(that);
    // Both lattices need to be talking about variables in the same function

	        if(that->allVarLattice) {
	                ROSE_ASSERT(allVarLattice);
	                this->allVarLattice->copy(that->allVarLattice);
	        }

	        // Iterate through all the lattices of constant variables, copying any lattices in That to This
	        for(map<varID, Lattice*>::iterator var=that->constVarLattices.begin(); var!=that->constVarLattices.end(); var++) {

	        		if(constVarLattices.find(var->first) != constVarLattices.end()) {
	                        ROSE_ASSERT(constVarLattices[var->first]);
	                        constVarLattices[var->first]->copy(var->second);
	                } else {
	                        ROSE_ASSERT(var->second);
	                        constVarLattices.insert(make_pair(var->first, var->second->copy()));
	                }
	        }

	        // Iterate through all the variables mapped by this lattice, copying any lattices in That to This
	        for(map<varID, int>::iterator var = that->varLatticeIndex.begin(); var != that->varLatticeIndex.end(); var++)
	        {
	                if(f(that->lattices[var->second])) {
	                	continue;
	                }
	        		if(varLatticeIndex.find(var->first) != varLatticeIndex.end()) {
	                        ROSE_ASSERT(lattices[varLatticeIndex[var->first]]);
	                        lattices[varLatticeIndex[var->first]]->copy(that->lattices[var->second]);
	                } else {
	                        varLatticeIndex[var->first] = lattices.size();
	                        ROSE_ASSERT(that->lattices[var->second]);
	                        lattices.push_back(that->lattices[var->second]->copy());
	                }
	        }
}
