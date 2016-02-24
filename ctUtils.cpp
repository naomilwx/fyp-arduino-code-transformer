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
