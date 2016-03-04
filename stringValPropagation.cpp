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
	FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(*(NodeState::getLatticeBelow(this, n, 0).begin()));
	return dynamic_cast<StringValLattice *>(lat->getVarLattice(varID(var)));
}

StringValLattice *StringValPropagation::getValLattice(NodeState *s, varID var) {
	FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(*(s->getLatticeBelow(this).begin()));
	return dynamic_cast<StringValLattice *>(lat->getVarLattice(var));
}
bool StringValPropagation::isModifiedStringRef(SgFunctionDefinition *def, SgVarRefExp *var) {
	DataflowNode n = cfgUtils::getFuncEndCFG(def, filter);
	NodeState *state = NodeState::getNodeState(n, n.getIndex());
	FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(*(state->getLatticeBelow(this).begin()));
	return dynamic_cast<StringValLattice *>(lat->getVarLattice(varID(var)))->getLevel() == StringValLattice::TOP;
}

void StringValPropagation::runAnalysis() {
	 SgIncidenceDirectedGraph *graph = buildProjectCallGraph(project);
	 ContextInsensitiveInterProceduralDataflow inter(this, graph);
	 inter.runAnalysis();
}
