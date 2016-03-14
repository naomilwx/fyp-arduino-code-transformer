/*
 * ctOverallDataflowAnalyser.h
 *
 *  Created on: Mar 14, 2016
 *      Author: root
 */

#ifndef CTOVERALLDATAFLOWANALYSER_H_
#define CTOVERALLDATAFLOWANALYSER_H_

#include "ctUtils.h"

#include "rose.h"
#include "dataflow.h"
#include "analysis.h"
#include "VariableStateTransfer.h"


class ctOverallDataflowAnalyser: public virtual InterProceduralDataflow {
	protected:
		SgProject *project;
		//	std::map<SgFunctionDeclaration *, std::vector<FunctionDataflowInfo>> functionRetInfo;


	public:
		ctOverallDataflowAnalyser(SgProject *project, IntraUniDirectionalDataflow *analyser);
		void runAnalysis();
		bool transfer(const Function& func, const DataflowNode& n, NodeState& state,
				const std::vector<Lattice*>& dfInfo, std::vector<Lattice*>** retState, bool fw);
};


#endif /* CTOVERALLDATAFLOWANALYSER_H_ */
