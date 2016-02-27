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

void StringValPropagationTransfer::visit(SgPntrArrRefExp *n) {
	StringValLattice* lattice = getLattice(n->get_lhs_operand());
	lattice->setLevel(StringValLattice::TOP);
}

void StringValPropagationTransfer::visit(SgFunctionCallExp *n){
	//TODO: figure out which arguments are mutable and propagate accordingly
	SgExpression *funcRef = getFunctionRef(n);
	SgExpressionPtrList params = n->get_args()->get_expressions();
	if(funcRef != NULL) {
		SgFunctionType *funcType = dynamic_cast<SgFunctionType *>(funcRef->get_type());
		SgTypePtrList fArgs = funcType->get_arguments();
		int argIdx = 0;
		for(auto &fArg : fArgs) {
			if(fArg->containsInternalTypes() && isConstantType(fArg) == false) {
				StringValLattice* lattice = getLattice(params[argIdx]);
				lattice->setLevel(StringValLattice::TOP);
			}
			argIdx++;
//			printf("function arg type: %s\n", fArg->class_name().c_str());
		}
//		printf("type ref %s\n", funcType->class_name().c_str());
	}

//	printf("call type: %s\n", n->get_type()->class_name().c_str());
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
