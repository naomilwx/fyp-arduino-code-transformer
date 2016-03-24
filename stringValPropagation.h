#ifndef _STRINGVALPROPAGATION_H_
#define _STRINGVALPROPAGATION_H_
#include "rose.h"
#include "dataflow.h"
#include "VariableStateTransfer.h"
#include "analysis.h"
#include "boost/shared_ptr.hpp"
#include "boost/unordered_map.hpp"
#include "boost/foreach.hpp"

#include "ctUtils.h"
#include "stringValLattice.h"
#include "stringLiteralAnalysis.h"
#include "ctOverallDataflowAnalyser.h"


extern int PointerAliasAnalysisDebugLevel;

class PointerAliasAnalysis;

/*
Transfer:  Visit functions for SgFunctinCallExp, SgAssignOp, SgAssignInitializer, SgConstructorInitializer are defined
i.e., the CFG nodes that could potentially update any pointers.
processLHS() and processRHS() functions are used to find the AliasRelations at a CFG node, using the LHS and RHS of 
the given expression. 
updateAliases() is used to update the compact representation graph, which is nothing but the set of Aliases at each CFG node.
computeAliases() is used to compute the Aliases for a given variable, using the compact representation graph 
 */
class PointerAliasAnalysisTransfer : public VariableStateTransfer<PointerAliasLattice>
{
	private:
		using VariableStateTransfer<PointerAliasLattice>::getLattices;
		LiteralMap *literalMap;
		PointerAliasAnalysis* analysis;
	public:

		void visit(SgFunctionCallExp *sgn);
		void visit(SgAssignOp *sgn);
		void visit(SgAssignInitializer *sgn);
		void visit(SgConstructorInitializer *sgn);
		void visit(SgAggregateInitializer *sgn);        
		void visit(SgFunctionDefinition *def);
		void visit(SgExpression *expr);
		bool finish();

		PointerAliasAnalysisTransfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo, LiteralMap *map, PointerAliasAnalysis* analysis);

		//processes LHS of an expression 'node' and populates 'arNode' with the varID and its derefCount
		static void processLHS(SgNode *node, struct aliasDerefCount &arNode);
		
		//processes RHS of an expression 'node' and populates 'arNode' with the varID and its derefCount
		static void processRHS(SgNode *node, struct aliasDerefCount &arNode, LiteralMap *map);

		void processRHS(SgNode *node, struct aliasDerefCount &arNode);

	private:
		std::vector<aliasDerefCount> getReturnAliasForFunctionCall(SgFunctionCallExp *fcall);


		void updateStateForAssignOp(PointerAliasLattice *lhsLat, SgExpression *lhs);

		//Updates the 'aliasedVariables' set by establishing an relation('edge' in compact representation graph) between 'aliasRelations' pair. 'isMust' denotes may or must alias
		bool updateAliases(set< std::pair<aliasDerefCount, aliasDerefCount> > aliasRelations,int isMust);

		void setAliasesForExpression(SgExpression *expr, std::vector<aliasDerefCount> aliases);

		//Recursive function to traverse the per-variable lattices to compute Aliases for 'var' at deref count of 'derefLevel'
		bool computeAliases(PointerAliasLattice *lat, varID var, int derefLevel, set<varID> &result);

		void processParam(int index, SgScopeStatement *scope, SgInitializedName *param, struct aliasDerefCount &arNode);

		void approximateFunctionCallEffect(SgFunctionCallExp *fcall);
		//Over approximates effect of function calls:
		//Assume all parameters passed as non const pointers or reference are modified

		void propagateFunctionCallEffect(SgFunctionCallExp *fcall);

		std::map<varID,varID> getPlaceholderToArgMap(SgFunctionCallExp *fcall);
}; 



class PointerAliasAnalysis : public IntraFWDataflow
{
public:

	protected:
		LiveDeadVarsAnalysis* ldva;
		LiteralMap *literalMap;
		SgProject *project;
		std::map<varID, Lattice*> globalVarsLattice;

	public:
		PointerAliasAnalysis(LiveDeadVarsAnalysis* ldva, SgProject *project, LiteralMap *map);

		PointerAliasLattice *getAliasLattice(NodeState *s, varID var);

		void genInitState(const Function& func, const DataflowNode& n, const NodeState& state,std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);
		bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

		boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& 
				n, NodeState& state, const std::vector<Lattice*>& dfInfo);

		bool runAnalysis(const Function &func, NodeState *fstate, bool analyzeDueToCallers, std::set<Function> calleesUpdated);
		void runAnalysis();
		void runGlobalVarAnalysis();
		void transferFunctionCall(const Function &func, const DataflowNode &n, NodeState *state);

		bool isUnmodifiedStringOrCharArray(SgFunctionDeclaration *func, SgNode *exp);
		bool isMultiAssignmentPointer(SgFunctionDeclaration *func, SgNode *exp);
		bool isStaticallyDeterminatePointer(SgFunctionDeclaration *func, SgNode *exp);
		bool isNotReassignedOrModified(SgFunctionDeclaration *func, SgNode *exp);

		bool variableAtNodeHasKnownAlias(SgNode *node, varID var);

		std::set<varID> getAliasesForVariableAtNode(SgNode *node, varID var);
		PointerAliasLattice *getReturnValueAliasLattice(SgFunctionDeclaration *func);
		PointerAliasLattice *getReturnValueAliasLattice(const Function& func);
	private:
		PointerAliasLattice *getReturnStateAliasLattice(SgFunctionDeclaration *func, SgNode *exp);
		ctVarsExprsProductLattice *getReturnStateLattice(SgFunctionDeclaration *func);
		void setGlobalAliasRelationForLat(PointerAliasLattice *lat, aliasDerefCount& lhs, SgNode *rhsExp);
		void computeGlobalAliases(PointerAliasLattice *lat, varID var, int derefLevel, set<varID> &result);

		static bool paaFilter(CFGNode cfgn) {
			SgNode *node = cfgn.getNode();
			SgNode *par = node;

			while(par != NULL && !isSgFunctionDefinition(par)) {
				par = par->get_parent();
			}

			if(!isSgFunctionDefinition(par)) {
				return false;
			}
			return defaultFilter(cfgn);
		}
};

#endif
