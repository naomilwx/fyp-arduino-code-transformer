/*
 * basicProgmemTransformer.h
 *
 *  Created on: Mar 24, 2016
 *      Author: root
 */

#ifndef BASICPROGMEMTRANSFORMER_H_
#define BASICPROGMEMTRANSFORMER_H_

#include "rose.h"
#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
//#include "convertibleFunctions.h"

//Assumes code has been transformed by codeSimplifier
class BasicProgmemTransformer {
protected:
	SgProject *project;
	PointerAliasAnalysis *aliasAnalysis;
	StringLiteralAnalysis *sla;

public:
	BasicProgmemTransformer(SgProject *p, PointerAliasAnalysis *aa, StringLiteralAnalysis* sla) {
		this->project = p;
		this->aliasAnalysis = aa;
		this->sla = sla;
	}
	void runTransformation();

private:
	int getBuffersizeNeededForFunction(SgFunctionDeclaration *func);
	void transformFunction(SgFunctionDeclaration *func);
	void shiftVarDeclsToProgmem();

	std::set<varID> getVarsBoundToNonPlaceholderPointers();
	std::set<varID> getVarsInUnsafeFunctionCalls();
	std::set<varID> getVarsReturnedByFunctions();
	std::set<varID> getProgmemablePlaceholders();
	std::vector<SgVariableDeclaration *> getProgmemableVarDecls();
};



#endif /* BASICPROGMEMTRANSFORMER_H_ */
