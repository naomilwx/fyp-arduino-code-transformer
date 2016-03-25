/*
 * basicProgmemTransformer.h
 *
 *  Created on: Mar 24, 2016
 *      Author: root
 */

#ifndef BASICPROGMEMTRANSFORM_H_
#define BASICPROGMEMTRANSFORM_H_

#include "rose.h"
#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"

//Assumes code has been transformed by codeSimplifier
class BasicProgmemTransform {
protected:
	SgProject *project;
	PointerAliasAnalysis *aliasAnalysis;
	StringLiteralAnalysis *sla;

public:
	BasicProgmemTransform(SgProject *p, PointerAliasAnalysis *aa, StringLiteralAnalysis* sla) {
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



#endif /* BASICPROGMEMTRANSFORM_H_ */
