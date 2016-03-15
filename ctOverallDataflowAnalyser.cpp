#include "boost/mem_fn.hpp"

#include "ctOverallDataflowAnalyser.h"

using std::auto_ptr;

ctOverallDataflowAnalyser::ctOverallDataflowAnalyser(SgProject *project, IntraUniDirectionalDataflow *analyser): InterProceduralAnalysis((IntraProceduralAnalysis *) analyser), InterProceduralDataflow((IntraProceduralDataflow *)analyser) {
	this->project = project;

	set<FunctionState*> allFuncs = FunctionState::getAllDefinedFuncs();
	for(set<FunctionState*>::iterator it=allFuncs.begin(); it!=allFuncs.end(); it++)
	{
		FunctionState* funcS = *it;
		if(funcS->func.get_definition()) {
			//DFStateAtReturns NEED REFERENCES TO vector<Lattice*>'S RATHER THAN COPIES OF THEM
			std::vector<Lattice *> empty;
			funcS->state.setLattices(analyser, empty);
			funcS->retState.setLattices(analyser, empty);
			funcS->state.addFact(analyser, 0, new DFStateAtReturns(funcS->state.getLatticeBelowMod((Analysis*)analyser),
						funcS->retState.getLatticeBelowMod((Analysis*)analyser)));
			Dbg::dbg << "Return state for function " << funcS << " " << funcS->func.get_name().getString() << endl
				<< "funcS->state" << funcS->state.str(analyser) << endl;
			//                                 << "funcS->retState="<<  funcS->retState.str(intraDataflowAnalysis) << endl;
		}
	}
}

bool ctOverallDataflowAnalyser::transfer(const Function& func, const DataflowNode& n, NodeState& state,
		const std::vector<Lattice*>& dfInfo, std::vector<Lattice*>** retState, bool fw){


	bool modified = false;
	SgFunctionCallExp* call = isSgFunctionCallExp(n.getNode());
	Function callee(call);
	ROSE_ASSERT(call);

	if(analysisDebugLevel > 0)
		Dbg::dbg << "ContextInsensitiveInterProceduralDataflow::transfer "
			<<func.get_name().getString()<<"()=>"<<callee.get_name().getString()<<"()\n";

	if(callee.get_definition()){
		FunctionState* funcS = FunctionState::getDefinedFuncState(callee);
		// The lattices before the function (forward: before=above, after=below; backward: before=below, after=above)
		const vector<Lattice*>* funcLatticesBefore;
		if(fw) funcLatticesBefore = &(funcS->state.getLatticeAbove((Analysis*)intraAnalysis));
		else   funcLatticesBefore = &(funcS->state.getLatticeBelow((Analysis*)intraAnalysis));

		// Update the function's entry/exit state       with the caller's state at the call site
		vector<Lattice*>::const_iterator itCalleeBefore, itCallerBefore;
		for(itCallerBefore = dfInfo.begin(), itCalleeBefore = funcLatticesBefore->begin();
				itCallerBefore!=dfInfo.end() && itCalleeBefore!=funcLatticesBefore->end();
				itCallerBefore++, itCalleeBefore++){
			Lattice* calleeL = *itCalleeBefore;
			Lattice* callerL = *itCallerBefore;

			if(analysisDebugLevel>=1) {
				Dbg::dbg << "      callerL=["<<calleeL<<"] "<<callerL->str("        ")<<endl;
				Dbg::dbg << "      Before calleeL=["<<calleeL<<"] "<<calleeL->str("        ")<<endl;
			}
			// Create a copy of the current lattice, remapped for the called function's variables
			Lattice* remappedL = callerL->copy();
			map<varID, varID> argParamMap;
			FunctionState::setArgParamMap(call, argParamMap);

			remappedL->remapVars(argParamMap, callee);

			Dbg::dbg << "      remappedL=["<<calleeL<<"] "<<remappedL->str("        ")<<endl;

			// update the callee's Lattice with the new information at the call site
			modified = calleeL->meetUpdate(remappedL) || modified;

			if(analysisDebugLevel>=1)
				Dbg::dbg << "      After modified = "<<modified
					<< "calleeL=["<<calleeL<<"] "<<calleeL->str("        ")<<endl;

		}

		// The lattices after the function (forward: before=above, after=below; backward: before=below, after=above).
		const vector<Lattice*>* funcLatticesAfter;
		if(fw) funcLatticesAfter = &(funcS->state.getLatticeBelow((Analysis*)intraAnalysis));
		else   funcLatticesAfter = &(funcS->state.getLatticeAbove((Analysis*)intraAnalysis));

		//Dbg::dbg << "        funcLatticesAfter->size()="<<funcLatticesAfter->size()<<endl;
		if(analysisDebugLevel>=1)
			Dbg::dbg << "      ----%%%%%%%%%%%%%%%%%%%%---------\n";

		// Transfer the result of the function call into the dfInfo Lattices.
		vector<Lattice*>::const_iterator itCalleeAfter, itCallerAfter;
		for(itCallerAfter = dfInfo.begin(), itCalleeAfter = funcLatticesAfter->begin();
				itCallerAfter!=dfInfo.end() && itCalleeAfter!=funcLatticesAfter->end();
				itCallerAfter++, itCalleeAfter++){
			Lattice* callerL = *itCallerAfter;
			Lattice* calleeL = *itCalleeAfter;
			//Dbg::dbg << "      calleeL-after=["<<calleeL<<"] "<<calleeL->str("        ")<<endl;

			// Create a copy of the current lattice, remapped for the callee function's variables
			Lattice* remappedL = calleeL->copy();
			if(analysisDebugLevel>=1)
				Dbg::dbg << "      remappedL-after=["<<remappedL<<"] "
					<<calleeL->str("        ")<<endl << remappedL->str(" ")<<endl;
			map<varID, varID> paramArgByRefMap;
			FunctionState::setParamArgByRefMap(call, paramArgByRefMap);

			remappedL->remapVars(paramArgByRefMap, func);

			//Dbg::dbg << "      callerL-after=["<<callerL<<"] "<<callerL->str("        ")<<endl;
			Dbg::dbg << "      +remappedL-after=["<<remappedL<<"] "<<remappedL->str("        ")<<endl;

			// update the caller's Lattice with the new information at the call site
			callerL->incorporateVars(remappedL);

			if(analysisDebugLevel>=1)
				Dbg::dbg << "      ==> callerL-after=["<<callerL<<"] "<<callerL->str("        ")<<endl;
			//Dbg::dbg << "      calleeL-after=["<<calleeL<<"] "<<calleeL->str("        ")<<endl;
			modified = true;
		}

		//		if(retState){
		//			*retState = &(funcS->retState.getLatticeBelowMod((Analysis*)intraAnalysis));
		//		}
		}
		return modified;
	}

	void ctOverallDataflowAnalyser::visit(const Function& func) {
		if(func.get_definition())
		{
			FunctionState* fState = FunctionState::getDefinedFuncState(func);
			assert(fState!=NULL);

			IntraProceduralDataflow *intraDataflow = dynamic_cast<IntraProceduralDataflow *>(intraAnalysis);
			assert(intraDataflow!=NULL);
			if (intraDataflow->visited.find(func) == intraDataflow->visited.end()) {
				vector<Lattice*>  initLats;
				vector<NodeFact*> initFacts;
				intraDataflow->genInitState(func, cfgUtils::getFuncStartCFG(func.get_definition(), filter),
						fState->state, initLats, initFacts);
				fState->state.setLattices(intraAnalysis, initLats);
				//				fState->state.setFacts(intraAnalysis, initFacts);
			}

			std::set<Function> emptyFunc;

			// Run the intra-procedural dataflow analysis on the current function
			dynamic_cast<IntraProceduralDataflow*>(intraAnalysis)->
				runAnalysis(func, &(fState->state), 
						false,
						emptyFunc);

			// Merge the dataflow states above all the return statements in the function, storing the results in Fact 0 of
			// the function
			DFStateAtReturns* dfsar = dynamic_cast<DFStateAtReturns*>(fState->state.getFact(intraAnalysis, 0));
			bool modified = dfsar->mergeReturnStates(func, fState, dynamic_cast<IntraProceduralDataflow*>(intraAnalysis));  

		}
	}
	void ctOverallDataflowAnalyser::runAnalysis() {
		//	std::vector<SgInitializedName *>globalVars = getGlobalVars(project);
		printf("begin analysis\n");
		FunctionSet funcs = getDefinedFunctions(project);
		SgIncidenceDirectedGraph *graph = buildProjectCallGraph(project);
		for(auto &func: funcs){
			FunctionState* fState = FunctionState::getFuncState(Function(func));
			printf("running\n");

			visit(func);

			printf("done running\n");
		}
	}
