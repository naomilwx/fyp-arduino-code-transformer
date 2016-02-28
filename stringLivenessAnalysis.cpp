#include "stringLivenessAnalysis.h"

//StringLivenessColouring
void StringLivenessColouring::genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts){
	initLattices.push_back(new LiveStringsFlowLattice());
	//	SgNode *node = n.getNode();
	//	printf("init %p %s\n %s\n", node, node->class_name().c_str(), node->unparseToString().c_str());
	//	auto test = NodeState::getNodeState(n, 0);
	//	printf("after first test %d\n", n.getIndex());
	//	NodeState *st = NodeState::getNodeState(node, 0);
}

boost::shared_ptr<IntraDFTransferVisitor> StringLivenessColouring::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo){
	return boost::shared_ptr<IntraDFTransferVisitor>(new StringLivenessColouringTransfer(func, n, state, dfInfo, slMap));
}

LiveStringsFlowLattice::FlowVal StringLivenessColouring::getFlowValue(const DataflowNode &n, const std::string& str){
	NodeState *state = NodeState::getNodeState(n, 0);
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(state->getLatticeBelow(this).begin()));
	return lat->getFlowValue(str);
}

LiveStringsFlowLattice::FlowVal StringLivenessColouring::getFlowValue(const NodeState &s, const std::string& str){
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(s.getLatticeBelow(this).begin()));
	return lat->getFlowValue(str);
}

bool StringLivenessColouring::isBeforeStringLiteral(const DataflowNode &n, const std::string& str){
	NodeState *state = NodeState::getNodeState(n, 0);
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


bool StringLivenessColouring::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo) {
	return false;
}

void StringLivenessColouring::runOverallAnalysis(SgProject *project) {
//	UnstructuredPassInterDataflow inter(this);
//	inter.runAnalysis();
	SgIncidenceDirectedGraph *graph = buildProjectCallGraph(project);
	ContextInsensitiveInterProceduralDataflow inter(this, graph);
	inter.runAnalysis();
	ranAnalysis = true;
}

void StringLivenessColouringTransfer::visit(SgStatement *n){
	if(slMap->find(n) != slMap->end()){
		StringSet strSet =  (*slMap)[n];
		for(std::string item: strSet){
			flowLattice->setFlowValue(item, LiveStringsFlowLattice::FlowVal::AFTER);
		}
		modified = true;
	}
}

bool StringLivenessColouringTransfer::finish() {
	return modified;
}

//StringLivenessAnalysis
void StringLivenessAnalysis::genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts){
	initLattices.push_back(new LiveStringsLattice());
}

boost::shared_ptr<IntraDFTransferVisitor> StringLivenessAnalysis::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo){
	return boost::shared_ptr<IntraDFTransferVisitor>(new StringLivenessAnalysisTransfer(func, n, state, dfInfo, valMappings, livenessColouring));
}

bool StringLivenessAnalysis::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo) {
	return false;
}

void StringLivenessAnalysis::runOverallAnalysis(SgProject *project) {
	if(livenessColouring-> hasRunAnalysis() == false) {
		livenessColouring->runOverallAnalysis(project);
	}
	SgIncidenceDirectedGraph *graph = buildProjectCallGraph(project);
	ContextInsensitiveInterProceduralDataflow inter(this, graph);
	inter.runAnalysis();
}

bool StringLivenessAnalysisTransfer::finish() {
//	SgNode *currNode = dfNode.getNode();
	bool changed = false;
	StringSet liveStringsSet = StringSet(liveStringsLat->getStrings());
	for(auto& str: liveStringsSet) {
		if(livenessColouring->isBeforeStringLiteral(nodeState, str) == true){
			liveStringsLat->remString(str);
			changed = true;
		}
	}
	for(auto& str: usedStrings){
		changed = liveStringsLat->addString(str) || changed;
	}
	return changed;
}

void StringLivenessAnalysisTransfer::visit(SgVarRefExp *ref) {
	if(varID::isValidVarExp(ref)== false) {
		return;
	}
	StringValLattice *lat = valMappings->getValLattice(dfNode.getNode(), ref);
	if(lat->getLevel() == StringValLattice::TOP) {
		return;
	}
	std::set<std::string> possibleVals = lat->getPossibleVals();
	for(auto val:possibleVals){
		liveStringsLat->addString(val);
	}
}

void StringLivenessAnalysisTransfer::visit(SgStringVal *val) {
	liveStringsLat->addString(val->get_value());
}
