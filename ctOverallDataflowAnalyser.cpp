#include "boost/mem_fn.hpp"

#include "ctOverallDataflowAnalyser.h"

using std::auto_ptr;

ctOverallDataflowAnalyser::ctOverallDataflowAnalyser(SgProject *project, IntraUniDirectionalDataflow *analyser): ContextInsensitiveInterProceduralDataflow((IntraProceduralDataflow *)analyser, buildProjectCallGraph(project)), InterProceduralAnalysis((IntraProceduralAnalysis *) analyser), InterProceduralDataflow((IntraProceduralDataflow *)analyser) {
	this->project = project;


}

bool ctOverallDataflowAnalyser::transfer(const Function& func, const DataflowNode& n, NodeState& state,
		const std::vector<Lattice*>& dfInfo, std::vector<Lattice*>** retState, bool fw){
	//TODO:
	return false;
}
void ctOverallDataflowAnalyser::runAnalysis() {
	//	std::vector<SgInitializedName *>globalVars = getGlobalVars(project);
	FunctionSet funcs = getDefinedFunctions(project);
	SgIncidenceDirectedGraph *graph = buildProjectCallGraph(project);
	for(auto &func: funcs){
		FunctionState* fState = FunctionState::getFuncState(Function(func));
		printf("running\n");
		CGFunction cgfunc(func, graph);
		visit(&cgfunc);
		//			std::vector<FunctionDataflowInfo> retInfo;
		//			Rose_STL_Container<SgNode *> returnStmts = NodeQuery::querySubTree(func, V_SgReturnStmt);
		//			for(Rose_STL_Container<SgNode*>::const_iterator i = returnStmts.begin(); i != returnStmts.end(); ++i) {
		//				SgReturnStmt *returnStmt = isSgReturnStmt(*i);
		//				FunctionDataflowInfo info;
		//				info.returnStmt = returnStmt;
		//				NodeState *ns = getNodeStateForNode(returnStmt, analyser->filter);
		//				auto res = ns->getLatticeBelow(analyser);
		//				info.lattice = dynamic_cast<FiniteVarsExprsProductLattice *>(*(res.begin()));
		//				retInfo.push_back(info);
		//			}
		//			functionRetInfo[func] = retInfo;
		printf("done running\n");
	}
}
