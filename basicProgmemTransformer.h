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

private:
	std::set<varID> getVarsBoundToNonPlaceholderPointers();
	std::set<varID> getVarsInUnsafeFunctionCalls();
	std::set<varID> getVarsReturnedByFunctions();
	std::set<SgVariableDeclaration *> getProgmemableVarDecls();
};



#endif /* BASICPROGMEMTRANSFORMER_H_ */
