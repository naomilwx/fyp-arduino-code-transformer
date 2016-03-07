#ifndef _STRINGVALPROPAGATION_H_
#define _STRINGVALPROPAGATION_H_
#include "rose.h"
#include "dataflow.h"
#include "VariableStateTransfer.h"
#include "analysis.h"
#include "boost/shared_ptr.hpp"

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
#endif
