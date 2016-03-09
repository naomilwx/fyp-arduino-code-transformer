#include "stringLivenessAnalysis.h"

class LivenessResultAnnotation : public AstSimpleProcessing {
StringLivenessAnalysis* analysis;
public:
	LivenessResultAnnotation(StringLivenessAnalysis* analysis) {
		this->analysis = analysis;
	}
	void visit(SgNode *node) {
		SgBasicBlock *block = isSgBasicBlock(node);
		if(block != NULL) {
			SgStatementPtrList stmts = block->get_statements();
			for(SgStatement * stmt: stmts) {
				LiveStringsLattice *liveIn = analysis->getLiveIn(stmt);
				LiveStringsLattice *liveOut = analysis->getLiveOut(stmt);
//				std::string cStr = analysis->getLivenessColouring()->getLatticeForNode(stmt)->str();
				std::string liveStr = "Live In: "+liveIn->str();
//				SageInterface::attachComment(stmt, cStr + "\n" +liveStr, PreprocessingInfo::before, PreprocessingInfo::C_StyleComment);
				SageInterface::attachComment(stmt, liveStr, PreprocessingInfo::before, PreprocessingInfo::C_StyleComment);
				SageInterface::attachComment(stmt, "Liveout:"+liveOut->str(), PreprocessingInfo::after, PreprocessingInfo::C_StyleComment);
			}
		}
	}

	void runAnnotations() {
		this->traverseInputFiles(analysis->getProject(), preorder);
	}
};

class StringLivenessHelper : public AstSimpleProcessing {
	StringLivenessColouring* colouring;
	StringSet startingStrs;
public:
	StringLivenessHelper(StringLivenessColouring *colouring, StringSet strs) {
		this->colouring = colouring;
		startingStrs = strs;
	}
	void visit(SgNode *node) {
		printf("visited %p %s\n", node, node->class_name().c_str());
		LiveStringsFlowLattice *lat = colouring->getLatticeForNode(node);
		for(std::string item: startingStrs){
			lat->setFlowValue(item, LiveStringsFlowLattice::FlowVal::SOURCE);
		}
	}
};

//StringLivenessColouring
void StringLivenessColouring::genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts){
	initLattices.push_back(new LiveStringsFlowLattice());
}

boost::shared_ptr<IntraDFTransferVisitor> StringLivenessColouring::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo){
	return boost::shared_ptr<IntraDFTransferVisitor>(new StringLivenessColouringTransfer(func, n, state, dfInfo, slMap, this));
}

LiveStringsFlowLattice::FlowVal StringLivenessColouring::getFlowValue(const NodeState &s, const std::string& str){
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(s.getLatticeBelow(this).begin()));
	return lat->getFlowValue(str);
}

LiveStringsFlowLattice* StringLivenessColouring::getLatticeForNode(SgNode *n) {
	NodeState *state = getNodeStateForNode(n, this->filter);
	return getLatticeForNodeState(*state);
}
LiveStringsFlowLattice* StringLivenessColouring::getLatticeForNodeState(const NodeState &s) {
	auto res = s.getLatticeBelow(this);
	return dynamic_cast<LiveStringsFlowLattice *>(*(res.begin()));
}

bool StringLivenessColouring::isBeforeStringLiteral(const NodeState &s, const std::string& str){
	LiveStringsFlowLattice *lat = getLatticeForNodeState(s);
	assert(lat != NULL);
	return lat->isBeforeStringLiteral(str);
}

//bool StringLivenessColouring::isBeforeStringVar(const NodeState &s, varID var){
//	LiveStringsFlowLattice *lat = getLatticeForNodeState(s);
//	assert(lat != NULL);
//	return lat->isBeforeStringVar(var);
//}

bool StringLivenessColouring::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo) {
	return false;
}

void StringLivenessColouring::runOverallAnalysis() {
	if(ranAnalysis == true) {
		return;
	}
	SgIncidenceDirectedGraph *graph = buildProjectCallGraph(project);
	ContextInsensitiveInterProceduralDataflow inter(this, graph);
	inter.runAnalysis();
	ranAnalysis = true;
}

void StringLivenessColouringTransfer::visit(SgStatement *n){
	if(slMap->find(n) != slMap->end()){
		StringSet strSet =  (*slMap)[n];
		StringLivenessHelper helper(livenessColouring, strSet);
		printf("start\n");
		helper.traverse(n, preorder);
		printf("end\n");
		modified = true;
	}
}

bool StringLivenessColouringTransfer::finish() {
	return modified;
}

//StringLivenessAnalysis
void StringLivenessAnalysis::runAnnotation() {
	LivenessResultAnnotation annot(this);
	annot.runAnnotations();
}
void StringLivenessAnalysis::genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts){
	initLattices.push_back(new LiveStringsLattice());
}

boost::shared_ptr<IntraDFTransferVisitor> StringLivenessAnalysis::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo){
	return boost::shared_ptr<IntraDFTransferVisitor>(new StringLivenessAnalysisTransfer(func, n, state, dfInfo, valMappings, livenessColouring));
}

LiveStringsLattice *StringLivenessAnalysis::getLiveIn(SgStatement *n)  const{
	NodeState* state = getNodeStateForNode(n, this->filter);
	return dynamic_cast<LiveStringsLattice *>(*(state->getLatticeAbove(this).begin()));
}

LiveStringsLattice *StringLivenessAnalysis::getLiveOut(SgStatement *n) const {
	NodeState* state = getNodeStateForNode(n, this->filter);
	return dynamic_cast<LiveStringsLattice *>(*(state->getLatticeBelow(this).begin()));
}

bool StringLivenessAnalysis::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo) {
	return false;
}

void StringLivenessAnalysis::runOverallAnalysis() {
	if(ranAnalysis == true) {
		return;
	}
	if(livenessColouring-> hasRunAnalysis() == false) {
		livenessColouring->runOverallAnalysis();
	}
	SgIncidenceDirectedGraph *graph = buildProjectCallGraph(project);
	ContextInsensitiveInterProceduralDataflow inter(this, graph);
	inter.runAnalysis();
	ranAnalysis = false;
}

bool StringLivenessAnalysisTransfer::finish() {
	bool changed = false;
	StringSet liveStringsSet = StringSet(liveStringsLat->getStrings());
	for(auto& str: liveStringsSet) {
		if(livenessColouring->isBeforeStringLiteral(nodeState, str) == true){
			liveStringsLat->remString(str);
			changed = true;
		}
	}

	for(auto &var: assignedVars) {
		changed = liveStringsLat->remStringVar(var) || changed;
	}

	for(auto& str: usedStrings){
		changed = liveStringsLat->addString(str) || changed;
	}
	for(auto& var:usedVars) {
		changed = liveStringsLat->addStringVar(var) || changed;
	}
	return changed;
}

void StringLivenessAnalysisTransfer::visit(SgVarRefExp *ref) {
	if(isSgAssignOp(ref->get_parent()) && ref->isLValue()) {
		if(varID::isValidVarExp(ref)) {
			assignedVars.insert(varID(ref));
		}
		return;
	}
	if(varID::isValidVarExp(ref)== false) {
		printf("rej %p %s\n", ref, ref->get_parent()->unparseToString().c_str());

		return;
	}
	StringValLattice *lat = valMappings->getValLattice(dfNode.getNode(), ref);
//	if(lat->getLevel() == StringValLattice::TOP || lat->getLevel() == StringValLattice::BOTTOM) {
//		return;
//	}
	if(lat->getLevel() == StringValLattice::BOTTOM) {
		return;
	}
//	if(lat->getLevel() == StringValLattice::CONSTANT) {
//		usedStrings.insert(*(lat->getPossibleVals().begin()));
//	} else {
		usedVars.insert(varID(ref));
//	}

}

void StringLivenessAnalysisTransfer::visit(SgStringVal *val) {
	usedStrings.insert(val->get_value());
}

void StringLivenessAnalysisTransfer::visit(SgInitializedName *init) {
	assignedVars.insert(varID(init));
}

