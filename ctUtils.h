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

bool isFromLibrary(SgInitializedName* initName);

FunctionSet getDefinedFunctions(SgProject *project);

SgExpression *getFunctionRef(SgFunctionCallExp *call);

bool isConstantType(SgType *nType);

SgIncidenceDirectedGraph * buildProjectCallGraph(SgProject *project);

unsigned int getNodeDataflowIndex(SgNode *n);

NodeState *getNodeStateForNode(SgNode *n, bool (*f) (CFGNode));

NodeState *getNodeStateForDataflowNode(DataflowNode &n, unsigned int index);

bool isArduinoStringType(SgType *type);

std::vector<SgInitializedName *>getGlobalVars(SgProject *project);

#endif
