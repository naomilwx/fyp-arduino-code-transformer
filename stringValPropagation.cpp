#include "stringValPropagation.h"
#include "analysisCommon.h"
#include "ctUtils.h"
int debugLevel = 2;

StringValPropagationTransfer::StringValPropagationTransfer(const Function &func, const DataflowNode &n, NodeState &state, const std::vector<Lattice *>& dfInfo) : VariableStateTransfer<StringValLattice>(func, n, state, dfInfo, debugLevel) {
}

void StringValPropagationTransfer::visit(SgStringVal *n) {
//	printf("visited...string...\n");
	StringValLattice* lattice = getLattice(n);
	lattice->addPossibleVal(n->get_value());
}

void StringValPropagationTransfer::visit(SgVarRefExp *n) {
	StringValLattice* lattice = getLattice(n);
	lattice->setLevel(StringValLattice::TOP);
}

bool StringValPropagationTransfer::finish() {
	return modified;
}

//StringValPropagation Implementation

void StringValPropagation::genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts) {
	std::map<varID, Lattice*> m;
	initLattices.push_back(new FiniteVarsExprsProductLattice((Lattice *) new StringValLattice(), m, (Lattice *)NULL, NULL, n, state));
}

bool StringValPropagation::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo) {
	return false;
}

boost::shared_ptr<IntraDFTransferVisitor> StringValPropagation::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo){
	return boost::shared_ptr<IntraDFTransferVisitor>(new StringValPropagationTransfer(func, n, state, dfInfo));
}


StringValLattice *StringValPropagation::getValLattice(SgNode *n, SgNode *var){
	NodeState *state = NodeState::getNodeState(n, 0);
	FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(*(state->getLatticeBelow(this).begin()));
	return dynamic_cast<StringValLattice *>(lat->getVarLattice(varID(var)));
}

SgIncidenceDirectedGraph * StringValPropagation::buildCallGraph(SgProject *project) {
	if(callGraph != NULL) {
		return callGraph;
	}
	DefinedFunctionCollector definedFuncsCollector;
	definedFuncsCollector.traverseInputFiles(project, preorder);
	definedFuncsCollector.printDefinedFunctions();

	CallGraphBuilder cgb(project);
	cgb.buildCallGraph(definedFuncsFilter(definedFuncsCollector.getDefinedFuncs()));
	callGraph = cgb.getGraph();
	return callGraph;
}

void StringValPropagation::runAnalysis(SgProject *project) {
	 SgIncidenceDirectedGraph *graph = buildCallGraph(project);
	 ContextInsensitiveInterProceduralDataflow inter(this, graph);
	 inter.runAnalysis();
}
