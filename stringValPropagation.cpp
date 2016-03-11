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
	//TODO: this is wrong. array ref on the rhs should not be marked
	StringValLattice* lattice = getLattice(n->get_lhs_operand());
	modified = lattice->setLevel(StringValLattice::TOP) || modified;

}

void StringValPropagationTransfer::visit(SgInitializedName *n){
	VariableStateTransfer<StringValLattice>::visit(n);
		StringValLattice* lattice = getLattice(n);
		if(lattice && lattice->getLevel() == StringValLattice::BOTTOM) {
			SgType *type = n->get_type();
			if(isSgTypeChar(type)) {
				return;
			}
			if(isArduinoStringType(type)) {
				modified = lattice->setLevel(StringValLattice::TOP) || modified;
			} else if(isSgTypeChar(type->findBaseType())) {
				modified = lattice->setLevel(StringValLattice::INITIALISED) || modified;
			}

		}
}
//void StringValPropagationTransfer::visit(SgAssignOp *n){
//	VariableStateTransfer<StringValLattice>::visit(n);
//	SgExpression *rhs = n->get_rhs_operand ();
//	StringValLattice* lattice = getLattice(rhs);
//	if(lattice->getLevel() == StringValLattice::BOTTOM) {
//		lattice->setLevel(StringValLattice::TOP);
//	}
//}
void StringValPropagationTransfer::visit(SgFunctionCallExp *n){
	SgExpression *funcRef = getFunctionRef(n);
	SgExpressionPtrList params = n->get_args()->get_expressions();
	if(funcRef != NULL) {
		SgFunctionType *funcType = dynamic_cast<SgFunctionType *>(funcRef->get_type());
		SgTypePtrList fArgs = funcType->get_arguments();
		int argIdx = 0;
		for(auto &fArg : fArgs) {
			if(fArg->containsInternalTypes() && isConstantType(fArg) == false) {
				StringValLattice* lattice = getLattice(params[argIdx]);
				modified = lattice->setLevel(StringValLattice::TOP) || modified;
			}
			argIdx++;
		}
	}

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
