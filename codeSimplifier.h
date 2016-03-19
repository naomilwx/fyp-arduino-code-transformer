/*
 * codeSimplifier.h
 *
 *  Created on: Mar 18, 2016
 *      Author: root
 */

#ifndef CODESIMPLIFIER_H_
#define CODESIMPLIFIER_H_

#include "rose.h"
#include "stringValPropagation.h"

class SimplifyFunctionDeclaration {
protected:
	PointerAliasAnalysis *aliasAnalysis;
	StringLiteralAnalysis *sla;
	SgFunctionDeclaration *func;
	SgProject *project;
public:
	SimplifyFunctionDeclaration(PointerAliasAnalysis *a, StringLiteralAnalysis *s, SgFunctionDeclaration *f, SgProject *p){
		this->aliasAnalysis = a;
		this->sla = s;
		this->func = f;
		this->project = p;
	}

	void transformVarDecls();
	void tranformVarRefs();
	void transformGlobals();
	void transformAssignments();
	void removeStringLiterals();
	void runTransformation();
private:
	std::map<std::string, SgVariableDeclaration *> slPlaceholders;
	std::map<std::string, SgVariableDeclaration *> builtPlaceholders;
	std::set<varID> varsToReplace;
	std::set<SgInitializer *> ignoredInitializers;
	void runAssignmentTransformation(SgAssignOp *op);
	void runVarRefsTransformation(SgVarRefExp *var);
	void runStringLiteralsTransformation(SgStringVal *strVal);
	void runVarDeclTransfromation(SgInitializedName *initName);
	void insertStringPlaceholderDecls();
	SgExpression * lookupAlias(varID alias);
	SgVariableDeclaration* checkAndBuildStringPlaceholder(const std::string placeholder);
	void buildStringPlaceholders();
	SgVariableDeclaration* buildStringPlaceholder(const std::string& str, const std::string& placeholder);
	void replaceWithAlias(SgVarRefExp *var);
	bool isVarExprToReplace(SgExpression *expr);
};

class SimplifyOriginalCode {
protected:
	PointerAliasAnalysis *aliasAnalysis;
	StringLiteralAnalysis *sla;
	SgProject *project;
public:
	SimplifyOriginalCode(PointerAliasAnalysis *a, StringLiteralAnalysis* s, SgProject *p){
		this->aliasAnalysis = a;
		this->sla = s;
		this->project = p;
	};
	void runTransformation();
	void simplifyFunction(SgFunctionDeclaration *func);
};

#endif /* CODESIMPLIFIER_H_ */
