#include "stringLivenessAnalysis.h"

//StringLivenessColouring
void StringLivenessColouring::genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts){
	initLattices.push_back(new LiveStringsFlowLattice());
}

boost::shared_ptr<IntraDFTransferVisitor> StringLivenessColouring::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo){
	return boost::shared_ptr<IntraDFTransferVisitor>(new StringLivenessColouringTransfer(func, n, state, dfInfo, slMap));
}

LiveStringsFlowLattice::FlowVal StringLivenessColouring::getFlowValue(SgNode *n, const std::string& str){
	NodeState *state = NodeState::getNodeState(n, 0);
	LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(state->getLatticeBelow(this).begin()));
	return lat->getFlowValue(str);
}

bool StringLivenessColouring::isBeforeStringLiteral(SgNode *n, const std::string& str){
	NodeState *state = NodeState::getNodeState(n, 0);
		LiveStringsFlowLattice *lat = dynamic_cast<LiveStringsFlowLattice *>(*(state->getLatticeBelow(this).begin()));
		return lat->isBeforeStringLiteral(str);
}

bool StringLivenessColouring::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo) {
	return false;
}

void StringLivenessColouring::runOverallAnalysis() {
	UnstructuredPassInterDataflow inter(this);
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

void StringLivenessAnalysis::runOverallAnalysis() {
	if(livenessColouring-> hasRunAnalysis() == false) {
		livenessColouring->runOverallAnalysis();
	}
	UnstructuredPassInterDataflow inter(this);
	inter.runAnalysis();
}

bool StringLivenessAnalysisTransfer::finish() {
	SgNode *currNode = dfNode.getNode();
	bool changed = false;
	for(auto& str: liveStringsLat->getStrings()) {
		if(livenessColouring->isBeforeStringLiteral(currNode, str)){
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
