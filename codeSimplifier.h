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
public:
	SimplifyFunctionDeclaration(PointerAliasAnalysis *a, StringLiteralAnalysis *s, SgFunctionDeclaration *f){
		this->aliasAnalysis = a;
		this->sla = s;
		this->func = f;
	}

	void transformVarDecls();
	void tranformVarRefs();
	void transformAssignments();
	void removeStringLiterals();
	void runTransformation();
private:
	std::map<std::string, SgVariableDeclaration *> slPlaceholders;
	std::set<varID> varsToReplace;
	std::set<SgInitializer *> ignoredInitializers;
	void runAssignmentTransformation(SgAssignOp *op);
	void runVarRefsTransformation(SgVarRefExp *var);
	void runStringLiteralsTransformation(SgStringVal *strVal);
	void runVarDeclTransfromation(SgInitializedName *initName);
	void insertStringPlaceholderDecls();
	void buildStringPlaceholders(SgScopeStatement *scope);
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
