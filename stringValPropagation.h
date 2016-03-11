#ifndef _STRINGVALPROPAGATION_H_
#define _STRINGVALPROPAGATION_H_
#include "rose.h"
#include "dataflow.h"
#include "VariableStateTransfer.h"
#include "analysis.h"
#include "boost/shared_ptr.hpp"
#include "boost/unordered_map.hpp"
#include "boost/foreach.hpp"

#include "ctUtils.h"
#include "stringValLattice.h"

class StringValPropagation : public IntraFWDataflow {
protected:
	SgProject *project;
	public:
	StringValPropagation(SgProject *project){
		this->project = project;
	}

	void genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);

    bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

    boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

    StringValLattice *getValLattice(SgNode *n, SgNode *var);
    StringValLattice *getValLattice(NodeState *s, varID var);

    bool isModifiedStringRef(SgFunctionDefinition *def, SgVarRefExp *ref);

    void runAnalysis();
};

class StringValPropagationTransfer : public VariableStateTransfer<StringValLattice> {
public:
	StringValPropagationTransfer(const Function &func, const DataflowNode &n, NodeState &state, const std::vector<Lattice *>& dfInfo);
	void visit(SgStringVal *n);
	void visit(SgPntrArrRefExp *n);
	void visit(SgFunctionCallExp *n);
//	void visit(SgAssignOp *n);
	void visit(SgInitializedName *n);
	bool finish();
};


extern int PointerAliasAnalysisDebugLevel;



/*
    Transfer:   We define visit functions for SgFunctinCallExp, SgAssignOp, SgAssignInitializer, SgConstructorInitializer
                i.e., the CFG nodes that could potentially update any pointers.
                processLHS() and processRHS() functions are used to find the AliasRelations at a CFG node, using the LHS and RHS of 
                the given expression. 
                updateAliases() is used to update the compact representation graph, which is nothing but the set of Aliases at each CFG node.
                computeAliases() is used to compute the Aliases for a given variable, using the compact representation graph 
*/
class PointerAliasAnalysisTransfer : public VariableStateTransfer<PointerAliasLattice>
{
    private:
          using VariableStateTransfer<PointerAliasLattice>::getLattices;

    public:
          //Visit function to apply "transfer" on the specified SgNode in CFG
          void visit(SgFunctionCallExp *sgn);
          void visit(SgAssignOp *sgn);
          void visit(SgAssignInitializer *sgn);
          void visit(SgConstructorInitializer *sgn);
        
          bool finish();
          PointerAliasAnalysisTransfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
    private:
         
          //processes LHS of an expression 'node' and populates 'arNode' with the varID and its derefCount
          void processLHS(SgNode *node, struct aliasDerefCount &arNode);


          //processes RHS of an expression 'node' and populates 'arNode' with the varID and its derefCount
          void processRHS(SgNode *node, struct aliasDerefCount &arNode);
          
          //Updates the 'aliasedVariables' set by establishing an relation('edge' in compact representation graph) between 'aliasRelations' pair. 'isMust' denotes may or must alias
          bool updateAliases(set< std::pair<aliasDerefCount, aliasDerefCount> > aliasRelations,int isMust);

          //Recursive function to traverse the per-variable lattices to compute Aliases for 'var' at deref count of 'derefLevel'
          void computeAliases(PointerAliasLattice *lat, varID var, int derefLevel, set<varID> &result);
}; 



class PointerAliasAnalysis : public IntraFWDataflow
{
protected:
    LiveDeadVarsAnalysis* ldva;

public:
    PointerAliasAnalysis(LiveDeadVarsAnalysis* ldva);
    void genInitState(const Function& func, const DataflowNode& n, const NodeState& state,std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);
    bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
    boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& 
n, NodeState& state, const std::vector<Lattice*>& dfInfo);
};

#endif
