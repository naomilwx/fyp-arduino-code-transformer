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
//				printf("stmt %s %p \n", stmt->class_name().c_str(), stmt);
				LiveStringsLattice *liveIn = analysis->getLiveIn(stmt);
				LiveStringsLattice *liveOut = analysis->getLiveIn(stmt);
				SageInterface::attachComment(stmt, "Live In: "+liveIn->str()+"\n Liveout:"+liveOut->str(), PreprocessingInfo::before, PreprocessingInfo::C_StyleComment);
			}
		}
	}

	void runAnnotations() {
		this->traverseInputFiles(analysis->getProject(), preorder);
	}
};

//StringLivenessColouring
void StringLivenessColouring::genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts){
	initLattices.push_back(new LiveStringsFlowLattice());
//	SgNode *node = n.getNode();
//	printf("init %p %s\n", node, node->class_name().c_str());
//		if(node->cfgIndexForEnd() != n.getIndex()){
//		printf("init %p %s\n %s %d\n", node, node->class_name().c_str(), node->unparseToString().c_str(), node->cfgIndexForEnd());
	//	auto test = NodeState::getNodeState(n, 0);
//		assert(node->cfgIndexForEnd() == n.getIndex());
//	printf("node index %d\n", n.getIndex());
	//	NodeState *st = NodeState::getNodeState(node, 0);
}

boost::shared_ptr<IntraDFTransferVisitor> StringLivenessColouring::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo){
	return boost::shared_ptr<IntraDFTransferVisitor>(new StringLivenessColouringTransfer(func, n, state, dfInfo, slMap));
}

LiveStringsFlowLattice::FlowVal StringLivenessColouring::getFlowValue(const DataflowNode &n, const std::string& str){
	NodeState *state = NodeState::getNodeState(n, n.getIndex());
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(state->getLatticeBelow(this).begin()));
	return lat->getFlowValue(str);
}

LiveStringsFlowLattice::FlowVal StringLivenessColouring::getFlowValue(const NodeState &s, const std::string& str){
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(s.getLatticeBelow(this).begin()));
	return lat->getFlowValue(str);
}

bool StringLivenessColouring::isBeforeStringLiteral(const DataflowNode &n, const std::string& str){
	NodeState *state = NodeState::getNodeState(n, n.getIndex());
	auto res = state->getLatticeBelow(this);
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(res.begin()));
	assert(lat != NULL);
	return lat->isBeforeStringLiteral(str);
}

bool StringLivenessColouring::isBeforeStringLiteral(const NodeState &s, const std::string& str){
	auto res = s.getLatticeBelow(this);
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(res.begin()));
	assert(lat != NULL);
	return lat->isBeforeStringLiteral(str);
}

bool StringLivenessColouring::isBeforeStringVar(const NodeState &s, varID var){
	auto res = s.getLatticeBelow(this);
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(res.begin()));
	assert(lat != NULL);
	return lat->isBeforeStringVar(var);
}

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
		for(std::string item: strSet){
			flowLattice->setFlowValue(item, LiveStringsFlowLattice::FlowVal::SOURCE);
		}
		modified = true;
	}
}

void StringLivenessColouringTransfer::visit(SgInitializedName *n) {
	flowLattice->setFlowValue(varID(n), LiveStringsFlowLattice::FlowVal::SOURCE);
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
	unsigned int index = getNodeDataflowIndex(n);
	CFGNode cfgn(n, index);
	DataflowNode dfn(cfgn, this->filter);
	auto states = NodeState::getNodeStates(dfn);
	NodeState* state = (states.size() < (index + 1))? states[0] : states[index];
	return dynamic_cast<LiveStringsLattice *>(*(state->getLatticeAbove(this).begin()));
}

LiveStringsLattice *StringLivenessAnalysis::getLiveOut(SgStatement *n) const {
	unsigned int index = getNodeDataflowIndex(n);
	CFGNode cfgn(n, index);
	DataflowNode dfn(cfgn, this->filter);
	auto states = NodeState::getNodeStates(dfn);
	NodeState* state = (states.size() < (index + 1))? states[0] : states[index];
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
	for(auto& var:liveStringsLat->getLiveStringVars()) {
		StringValLattice *lat = valMappings->getValLattice(&nodeState, var);
		if(livenessColouring->isBeforeStringVar(nodeState, var) == true || lat->getLevel() == StringValLattice::CONSTANT) {
			liveStringsLat->remStringVar(var);
			changed = true;
		}
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
	if(varID::isValidVarExp(ref)== false) {
		return;
	}
	StringValLattice *lat = valMappings->getValLattice(dfNode.getNode(), ref);
	if(lat->getLevel() == StringValLattice::TOP || lat->getLevel() == StringValLattice::BOTTOM) {
		return;
	}
	if(lat->getLevel() == StringValLattice::CONSTANT) {
		usedStrings.insert(*(lat->getPossibleVals().begin()));
	} else {
		usedVars.insert(varID(ref));
	}
}

void StringLivenessAnalysisTransfer::visit(SgStringVal *val) {
	usedStrings.insert(val->get_value());
}


