#ifndef _STRINGVALLATTICE_H_
#define _STRINGVALLATTICE_H_
#include <string>
#include <set>
#include "latticeFull.h"

class StringValLattice: public FiniteLattice {
  public:
	enum ValLevel {
	  BOTTOM,
	  INITIALISED,
	  CONSTANT,
	  MODIFIED,
	  MULTIPLE,
	  TOP
	};
  protected:
	ValLevel level;
	std::set<std::string> possibleVals;
  
  public:
	StringValLattice(): level(BOTTOM), possibleVals() {}
	StringValLattice(const std::string& str);
	StringValLattice(const StringValLattice &lat);

	ValLevel getLevel() const;
	bool setLevel(ValLevel l);

	std::set<std::string> getPossibleVals() const;
	bool setPossibleVals(std::set<std::string> vals);
	bool setPossibleVals(std::string val);
	bool addPossibleVal(const std::string& val);
	
	bool setBottom();

	// **********************************************
	// Required definition of pure virtual functions.
	// **********************************************
	void initialize();
	std::string str(std::string indent="");

	Lattice* copy() const;
	void copy(Lattice* that);

	bool meetUpdate(Lattice *other);
	
	bool operator ==(Lattice *lat);
};


/*
    Pointer Alias Analysis
    aliasDerefCount : A struct that holds the alias information which includes the deference 
                Count for each pointer variable. E.g., int *p = &q would maintain a pair like so:
                pair( <p(SgVarSymbol), p(varID), 1> , < q(SgVariableSymbol), q(varID), -1 > ).
                aliasDerefCounts are used to create compact representation graphs for the pointers at 
                each CFG Node, by creating/removing an edge between the varID's of the left and right aliasDerefCounts
*/
struct aliasDerefCount{
    //! The VariableSymbol participating in aliasing
    SgVariableSymbol* var;
    varID vID;
    //! Dereference Level 
    //!  *a  +1
    //! **a  +2
    //!   a   0
    //!  &a  -1
    int derefLevel;

    bool operator==(const aliasDerefCount &that) const {
        assert(this != NULL);
        return (this->var == that.var && this->vID == that.vID && this->derefLevel == that.derefLevel);
    }

    bool operator<(const aliasDerefCount &that) const{
        assert(this != NULL);
        return true;
    }
};

/*
    Lattices:   Per variable lattices using the FiniteVarsExprProductLattice
                Each variable maintains a set<varID> and set< pair<aliasDerefCount,aliasDerefCount> >
                The set of all per-variable lattices is an abstraction of the compact representation graph, i.e, a structure to maintain
                the pointer alias information at each CFG Node.
    Example:    Consider the code:
                    int **x;
                    int *p,*q;
                    int a;
                    p = &a;
                    q = p;
                    x = &p;
                The aliases at the end of this piece of code would look something like this:
                p -> a
                q -> a
                x -> p
                The compact representation graph would be a graph containing an edge from 'x' to 'p', 'p' to 'a' and an edge from 'q' to 'a'
                To represent it as a lattice, we have per variable lattices like so:
                p: {a}
                q: {a}
                x: {p}
                where each variable such as 'p' and 'q' here contain a set<varID> as it aliases.
                Since each variable pointer is stored with a derefernce count in aliasDerefCount, we use that information to traverse the per variable lattices using a recursive algorithm called "computeAliases". For ex: derefernce 2 for variable x gives {a}
*/
class PointerAliasLattice : public FiniteLattice {
public:
	enum StateVal {
		BOTTOM,
		INITIALIZED,
		REASSIGNED,
		MODIFIED
	};
protected:
        //per variable aliases
        set<varID> aliasedVariables;

        //Set of all alias relations per CFG Node. These relations denote the graph edges in the compact represntation graph for pointers
        set< std::pair<aliasDerefCount, aliasDerefCount> > aliasRelations;
	StateVal state;
public:
        PointerAliasLattice():state(BOTTOM){};
        void initialize();
        Lattice* copy()const ;
        void copy(Lattice* that);
        bool operator==(Lattice*);

        bool setState(StateVal state);
        StateVal getState() {
        	return state;
        }

        bool meetUpdate(Lattice* that);
        std::string str(std::string);

        void setAliasedVariables(varID al);
        void clearAliasedVariables();
        void setAliasRelation(std::pair < aliasDerefCount, aliasDerefCount > alRel);
        set< std::pair<aliasDerefCount, aliasDerefCount> > getAliasRelations();
        set<varID> getAliasedVariables();
private:
        template <typename T> bool search(set<T> thisSet, T value);
};
#endif
