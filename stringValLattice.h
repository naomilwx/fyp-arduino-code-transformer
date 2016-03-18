#ifndef _STRINGVALLATTICE_H_
#define _STRINGVALLATTICE_H_
#include <string>
#include <set>
#include "latticeFull.h"
#include "liveDeadVarAnalysis.h"

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
		REASSIGNED_MULTIPLE,
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
        PointerAliasLattice(const PointerAliasLattice& lat): state(lat.state), aliasedVariables(lat.aliasedVariables), aliasRelations(lat.aliasRelations){
        }
        ~PointerAliasLattice() {
//        	printf("deleting %s\n", this->str(" ").c_str());
        }
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
        void setAliasedVariables(std::set<varID> als);
        void clearAliasedVariables();
        bool setAliasRelation(std::pair < aliasDerefCount, aliasDerefCount > alRel);
        set< std::pair<aliasDerefCount, aliasDerefCount> > getAliasRelations();
        set<varID> getAliasedVariables();
private:
        template <typename T> bool search(set<T> thisSet, T value);
};

/**
 * Modified version of FiniteVarsProductLattice
 * */

class ctVarsExprsProductLattice : public FiniteVarsExprsProductLattice {
        protected:
        // Minimal constructor that initializes just the portions of the object required to make an
        // initial blank VarsExprsProductLattice
		ctVarsExprsProductLattice(const DataflowNode& n, const NodeState& state);

        // Returns a blank instance of a VarsExprsProductLattice that only has the fields n and state set
        VarsExprsProductLattice* blankVEPL(const DataflowNode& n, const NodeState& state);

        public:
        // creates a new VarsExprsProductLattice
        // perVarLattice - sample lattice that will be associated with every variable in scope at node n
        //     it should be assumed that the object pointed to by perVarLattice will be either
        //     used internally by this VarsExprsProductLattice object or deallocated
        // constVarLattices - map of additional variables and their associated lattices, that will be
        //     incorporated into this VarsExprsProductLattice in addition to any other lattices for
        //     currently live variables (these correspond to various useful constant variables like zeroVar)
        // allVarLattice - the lattice associated with allVar (the variable that represents all of memory)
        //     if allVarLattice==NULL, no support is provided for allVar
        // func - the current function
        // n - the dataflow node that this lattice will be associated with
        // state - the NodeState at this dataflow node
        ctVarsExprsProductLattice(Lattice* perVarLattice,
                                     const std::map<varID, Lattice*>& constVarLattices,
                                     Lattice* allVarLattice,
                                     LiveDeadVarsAnalysis* ldva,
                                     const DataflowNode& n, const NodeState& state, std::map<varID, Lattice *> globalLattices);

        ctVarsExprsProductLattice(const ctVarsExprsProductLattice& that);

        Lattice* addSlotForVariable(varID var);

        void incorporateVarsMap(std::map<varID, Lattice *> lats, bool overwrite);
        // returns a copy of this lattice
        Lattice* copy() const;

        void incorporateVars(Lattice *that_arg, bool(*ignoreVarLattice)(Lattice*));
};
#endif
