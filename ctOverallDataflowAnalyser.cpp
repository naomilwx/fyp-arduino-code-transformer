#include "boost/mem_fn.hpp"

#include "ctOverallDataflowAnalyser.h"

using std::auto_ptr;

ctOverallDataflowAnalyser::ctOverallDataflowAnalyser(SgProject *project, IntraUniDirectionalDataflow *analyser): InterProceduralAnalysis((IntraProceduralAnalysis *) analyser), InterProceduralDataflow((IntraProceduralDataflow *)analyser) {
	this->project = project;

	// Record that the functions that have no callers are being analyzed because the data flow at their
	// callers (the environment) has changed. This is done to jump-start the analysis.
//	for(set<const CGFunction*>::iterator func=noPred.begin(); func!=noPred.end(); func++)
//		remainingDueToCallers.insert(**func);

	// Record as part of each FunctionState the merged lattice states above the function's return statements
	set<FunctionState*> allFuncs = FunctionState::getAllDefinedFuncs();
	for(set<FunctionState*>::iterator it=allFuncs.begin(); it!=allFuncs.end(); it++) {
		FunctionState* funcS = *it;
		if(funcS->func.get_definition()) {
			//DFStateAtReturns NEED REFERENCES TO vector<Lattice*>'S RATHER THAN COPIES OF THEM
			std::vector<Lattice *> empty;
			funcS->state.setLattices(analyser, empty);
			funcS->retState.setLattices(analyser, empty);
			funcS->state.addFact(this, 0, new DFStateAtReturns(funcS->state.getLatticeBelowMod((Analysis*)analyser),
						funcS->retState.getLatticeBelowMod((Analysis*)analyser)));
			Dbg::dbg << "Return state for function " << funcS << " " << funcS->func.get_name().getString() << endl
				<< "funcS->state" << funcS->state.str(analyser) << endl;
			//                                 << "funcS->retState="<<  funcS->retState.str(intraDataflowAnalysis) << endl;
		}
	}
}

bool ctOverallDataflowAnalyser::transfer(const Function& func, const DataflowNode& n, NodeState& state,
		const std::vector<Lattice*>& dfInfo, std::vector<Lattice*>** retState, bool fw){
	//TODO:
	return false;
}
void ctOverallDataflowAnalyser::runAnalysis() {
	//	std::vector<SgInitializedName *>globalVars = getGlobalVars(project);
	FunctionSet funcs = getDefinedFunctions(project);
	for(auto &func: funcs){
		FunctionState* fState = FunctionState::getFuncState(Function(func));
		printf("running\n");
		std::set<Function> calleesUpdated;
		dynamic_cast<IntraProceduralDataflow*>(intraAnalysis)->runAnalysis(fState->func, &(fState->state), true, calleesUpdated);
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
