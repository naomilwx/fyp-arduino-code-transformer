#include "dataflow.h"
#include "VariableStateTransfer.h"
#include "analysis.h"
#include "rose.h"

#include "boost/shared_ptr.hpp"

#include "stringValLattice.h"

class StringValPropagation : public IntraFWDataflow {
	public:
	StringValPropagation(){
		printf("init...\n");
	}

	void genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);

    bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

    boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

    StringValLattice *getValLattice(SgNode *n, SgNode *var);
};

class StringValPropagationTransfer : public VariableStateTransfer<StringValLattice> {
public:
	StringValPropagationTransfer(const Function &func, const DataflowNode &n, NodeState &state, const std::vector<Lattice *>& dfInfo);
	void visit(SgStringVal *n);
	bool finish();
};
