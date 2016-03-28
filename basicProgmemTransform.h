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
//TODO: shift unmodified arrays to progmem too...
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
	std::map<std::string, SgVariableDeclaration *> additionalProgmemStrings;

	int getBuffersizeNeededForFunction(SgFunctionDeclaration *func);
	void setupCharBufferForFunction(SgFunctionDeclaration *func);
	void transformFunction(SgFunctionDeclaration *func);
	void transformStringConstructors(SgFunctionDeclaration *func);
	void castProgmemParams(SgVarRefExp *var);
	void loadProgmemStringsIntoBuffer(SgFunctionCallExp *funcCall, SgVarRefExp *var, int& pos);
	void loadReplacewithProgmemFunction(SgFunctionCallExp *funcCall, std::string replacement);

	void transformCharArrayInitialization();
	int getDeclaredArraySize(SgArrayType *arrType);
	SgVariableDeclaration *getVariableDeclPlaceholderForString(const std::string& str);

	void shiftVarDeclsToProgmem();
	void convertVarDeclToProgmemDecl(SgVariableDeclaration *varDecl);

	std::set<varID> getVarsBoundToNonPlaceholderPointers();
	std::set<varID> getVarsInUnsafeFunctionCalls();
	std::set<varID> getVarsReturnedByFunctions();
	std::set<varID> getProgmemablePlaceholders();
	std::set<varID> getVarsInUnsafeConstructors();
	void setupProgmemableVarDecls();

	SgExpression *getRHSOfVarDecl(SgInitializedName *initName);
	bool isVarDeclToRemove(SgInitializedName *name);
	int getSizeNeededToLoadFromProgmem(SgVarRefExp *name);
};



#endif /* BASICPROGMEMTRANSFORM_H_ */
