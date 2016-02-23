#include "printAnalysisStates.h"

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
				return true;
			}
		}
		return false;
	}
};
