#include "ctUtils.h"
//#include "DefUseAnalysis.h"

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
		//printf("type %s\n", (*i)->class_name().c_str());
		SgModifierType* modifierType = isSgModifierType(*i);
		if (modifierType != NULL)  {
			isConst = modifierType->get_typeModifier().get_constVolatileModifier().isConst() || isConst;
		}
	}
	return isConst;
}

FunctionSet getDefinedFunctions(SgProject *project) {
	static std::map<SgProject *, FunctionSet> definedFunctions;
	if(definedFunctions.find(project) != definedFunctions.end()) {
		return definedFunctions[project];
	}
	DefinedFunctionCollector definedFuncsCollector;
	definedFuncsCollector.traverseInputFiles(project, preorder);
	definedFunctions[project] = definedFuncsCollector.getDefinedFuncs();
	return definedFunctions[project];
}

SgIncidenceDirectedGraph * buildProjectCallGraph(SgProject *project) {
	static std::map<SgProject *, SgIncidenceDirectedGraph*> callGraphs;
	//if(callGraph != NULL) {
	if(callGraphs.find(project) != callGraphs.end()){	
		return callGraphs[project];
	}
	//	DefinedFunctionCollector definedFuncsCollector;
	//	definedFuncsCollector.traverseInputFiles(project, preorder);
	//	definedFuncsCollector.printDefinedFunctions();

	CallGraphBuilder cgb(project);
	cgb.buildCallGraph(definedFuncsFilter(getDefinedFunctions(project)));
	SgIncidenceDirectedGraph *callGraph = cgb.getGraph();
	callGraphs[project] = callGraph;
	return callGraph;
}

unsigned int getNodeDataflowIndex(SgNode *n) {
	unsigned int index = 1; //0: entry, 1: function body, 3: exit, 2: partial expr? TODO: confirm this
	if(isSgConstructorInitializer(n)) {
		index = 2;
	} else if(isSgFunctionCallExp(n)) {
		index = 3;
	} else if(isSgExprListExp(n)){
		index = n->cfgIndexForEnd();
	} else if(isSgVarRefExp(n)){
		index = 0;
	} else if(isSgAssignOp(n)){
		index = 2;
	} else if(isSgDotExp(n)){
		index = 2;
	} else if(isSgMemberFunctionRefExp(n)){
		index = 0;
	} else if(isSgExprStatement(n)) {
		index = 1;
	} else if(isSgFunctionRefExp(n)) {
		index = 0;
	} else if(isSgFunctionDefinition(n)){
		index = 0;
	} else if(isSgScopeStatement(n)) {
		index = 1;
	} else if(isSgVariableDeclaration(n)) {
		index = 1;
	} else if(isSgDeclarationStatement(n)) {
		index = 0;
	}
	return index;
}


NodeState *getNodeStateForNode(SgNode *n, bool (*f) (CFGNode)){
	unsigned int index = getNodeDataflowIndex(n);
	CFGNode cfgn(n, index);
	DataflowNode dfn(cfgn, f);
	return getNodeStateForDataflowNode(dfn, index);
}

NodeState *getNodeStateForDataflowNode(DataflowNode &n, unsigned int index){
	auto states = NodeState::getNodeStates(n);
	//	NodeState* state = (states.size() < (index + 1))? states[0] : states[index];
	if( states.size() < (index + 1)) {
		return states[0];
	} else {
		return states[index];
	}
}

bool isFromLibrary(SgInitializedName* initName) {
  Sg_File_Info* fi = initName->get_file_info();
  if (fi->isCompilerGenerated())
    return true;
  string filename = fi->get_filenameString();
  if ((filename.find("/include/") != std::string::npos) || (filename.find("Arduino/hardware") != std::string::npos) || (filename.find("NULL_FILE") != std::string::npos))
      return true;
  return false; 
}

std::vector<SgInitializedName *>getGlobalVars(SgProject *project) {
	static std::map<SgProject *, std::vector<SgInitializedName *>> globalVarsMap;
	if(globalVarsMap.find(project) != globalVarsMap.end()){
		return globalVarsMap[project];
	}
	std::vector<SgInitializedName *>globalVars;	
	Rose_STL_Container<SgNode*> initNames = NodeQuery::querySubTree(project, V_SgInitializedName);
	for (Rose_STL_Container<SgNode*>::const_iterator i = initNames.begin(); i != initNames.end(); ++i) {
		SgInitializedName* iName = isSgInitializedName(*i);
		if  (!isFromLibrary(iName) && isSgGlobal(iName->get_scope()) != NULL) {
			globalVars.push_back(iName);
			printf("glob %s\n",iName->get_name().str());
		}
	}
	globalVarsMap[project] = globalVars;
	printf("number of globals %lu\n", globalVars.size());
	return globalVars;
}

bool isArduinoStringType(SgType *type) {
	SgNamedType *named = isSgNamedType(type);
	if(named != NULL && named->get_name() == "String") {
		return true;
	}
	return false;
}

