#ifndef _CTUTILS_H_
#define _CTUTILS_H_

#include "printAnalysisStates.h"
#include "Ast.h"

typedef std::set<SgFunctionDeclaration *> FunctionSet;

void printAnalysis(Analysis *a, bool bw);

class DefinedFunctionCollector : public AstSimpleProcessing {
protected:
	FunctionSet definedFuncs;
public:
	DefinedFunctionCollector() : definedFuncs() {}
	void visit(SgNode *n);
	FunctionSet getDefinedFuncs() const;
	void printDefinedFunctions();
};

SgIncidenceDirectedGraph * buildProjectCallGraph(SgProject *project, bool ignoreCache);
FunctionSet getDefinedFunctions(SgProject *project);
SgExpression *getFunctionRef(SgFunctionCallExp *call);

bool isFromLibrary(SgInitializedName* initName);
std::vector<SgInitializedName *>getGlobalVars(SgProject *project);

//bool isConstantType(SgType *nType);
bool isArduinoStringType(SgType *type);

unsigned int getNodeDataflowIndex(SgNode *n);
NodeState *getNodeStateForNode(SgNode *n, bool (*f) (CFGNode));
NodeState *getNodeStateForDataflowNode(DataflowNode &n, unsigned int index);

int getPointerLevel(SgType *type);

namespace FunctionAnalysisHelper {
	const std::string FUNC_PARAM_TAG_PREFIX =  "__function_param_";
	std::string getPlaceholderNameForArgNum(int num);
	int getFunctionParamNumberFromTag(const std::string& paramTag);
	bool isFunctionParamPlaceholder(const std::string& p);
	SgInitializedName *getFunctionParamForPlaceholder(SgFunctionDeclaration * decl, const std::string& p);
}

#endif
