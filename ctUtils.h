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

struct ROSE_DLL_API definedFuncsFilter : public std::unary_function<bool, SgFunctionDeclaration*> {
	FunctionSet definedFuncs;
	definedFuncsFilter(FunctionSet funcs): definedFuncs(funcs){}
	bool operator()(SgFunctionDeclaration *n) {
		for(auto const& func:  definedFuncs){
			if( n->get_name().getString() == func->get_name().getString()){
//				printf("%p, original: %p\n", n, func);
				return true;
			}
		}
		return false;
	}
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

struct FunctionDataflowInfo {
	SgReturnStmt *returnStmt;
	Lattice *lattice;
};

#endif
