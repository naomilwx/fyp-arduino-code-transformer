#include "ctUtils.h"

void printAnalysis(Analysis *a, bool bw) {
	vector<int> factNames;
	vector<int> latticeNames;
	latticeNames.push_back(0);
	printAnalysisStates::ab dir;
	if(bw) {
		dir = printAnalysisStates::above;
	}else {
		dir = printAnalysisStates::below;
	}
	printAnalysisStates pas(a, factNames, latticeNames, dir, " 	");
	UnstructuredPassInterAnalysis upas(pas);
	upas.runAnalysis();
}

FunctionSet DefinedFunctionCollector::getDefinedFuncs() const {
	return definedFuncs;
}

void DefinedFunctionCollector::visit(SgNode *n){
	SgFunctionDeclaration *func = isSgFunctionDeclaration(n);
	if(func != NULL){
//		printf("function ptr %p\n", func);
		if(func->get_definition() != NULL) {
			definedFuncs.insert(func);
		}
	}
}

void DefinedFunctionCollector::printDefinedFunctions() {
	printf("functions: [\n");
	for(auto const& func:  definedFuncs){
			printf("%s;;\n",func->get_name().getString().c_str());
	}
	printf("]\n");
}

SgExpression *getFunctionRef(SgFunctionCallExp *call) {
	SgExpression *funcRef = call->get_function();
	if(isSgFunctionRefExp(funcRef) != NULL || isSgMemberFunctionRefExp(funcRef) != NULL) {
		return funcRef;
	}
	if(isSgBinaryOp(funcRef) != NULL) {
		funcRef = isSgBinaryOp(funcRef)->get_rhs_operand();
	}
	Ast funcAst(funcRef);
	for(Ast::iterator it = funcAst.begin(); it!= funcAst.end(); it++) {
		SgNode* node = &(*it);
		if(isSgFunctionRefExp(node) != NULL) {
			return isSgFunctionRefExp(node);
		}
		if(isSgMemberFunctionRefExp(node) != NULL) {
			return isSgMemberFunctionRefExp(node);
		}
	}
	return NULL;
}

bool isConstantType(SgType *nType) {
	bool isConst = false;
	Rose_STL_Container<SgType*> typeVector = nType->getInternalTypes();
	for(Rose_STL_Container<SgType*>::iterator i = typeVector.begin(); i != typeVector.end(); i++){
	    SgModifierType* modifierType = isSgModifierType(*i);
	    if (modifierType != NULL)  {
	        isConst = modifierType->get_typeModifier().get_constVolatileModifier().isConst() || isConst;
	    }
	}
	return isConst;
}

SgIncidenceDirectedGraph * buildProjectCallGraph(SgProject *project) {
	static SgIncidenceDirectedGraph *callGraph = NULL;
	if(callGraph != NULL) {
		return callGraph;
	}
	DefinedFunctionCollector definedFuncsCollector;
	definedFuncsCollector.traverseInputFiles(project, preorder);
	definedFuncsCollector.printDefinedFunctions();

	CallGraphBuilder cgb(project);
	cgb.buildCallGraph(definedFuncsFilter(definedFuncsCollector.getDefinedFuncs()));
	callGraph = cgb.getGraph();
	return callGraph;
}
