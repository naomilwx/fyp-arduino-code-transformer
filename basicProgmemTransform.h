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
#include "ctUtils.h"

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

protected:
	void insertPreprocessingInfo(const std::string &data);

private:
	std::set<SgVariableDeclaration *> varDeclsToShift;

	int getBuffersizeNeededForFunction(SgFunctionDeclaration *func);
	void setupCharBufferForFunction(SgFunctionDeclaration *func);
	void transformFunction(SgFunctionDeclaration *func);
	void castProgmemParams(SgFunctionCallExp* funcCall, SgVarRefExp *var);
	void loadProgmemStringsIntoBuffer(SgFunctionDeclaration *caller, SgFunctionCallExp *funcCall, SgVarRefExp *var, int& pos);

	void shiftVarDeclsToProgmem();
	void convertVarDeclToProgmemDecl(SgVariableDeclaration *varDecl);

	std::set<varID> getVarsBoundToNonPlaceholderPointers();
	std::set<varID> getVarsInUnsafeFunctionCalls();
	std::set<varID> getVarsReturnedByFunctions();
	std::set<varID> getProgmemablePlaceholders();
	void setupProgmemableVarDecls();

	SgExpression *getRHSOfVarDecl(SgInitializedName *initName);
	bool isVarDeclToRemove(SgInitializedName *name);
	int getSizeNeededToLoadFromProgmem(SgVarRefExp *name);
};



#endif /* BASICPROGMEMTRANSFORM_H_ */
