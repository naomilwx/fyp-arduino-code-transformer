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
	void runTransformation(bool transformGlobals);
private:
	//map of string literal to the placeholder for the string
	std::map<std::string, SgVariableDeclaration *> slPlaceholders;
	 //map of the tags assigned to each string literal to the function variable declaration for the string literal
	std::map<std::string, SgVariableDeclaration *> builtPlaceholders;
	std::set<varID> varsToReplace;
	std::set<SgInitializer *> ignoredInitializers;
	void runAssignmentTransformation(SgAssignOp *op);
	void runVarRefsTransformation(SgVarRefExp *var);
	void runStringLiteralsTransformation(SgStringVal *strVal);
	void runVarDeclTransfromation(SgInitializedName *initName);
	void insertStringPlaceholderDecls();
	SgExpression * lookupAlias(varID alias);
	SgVariableDeclaration* checkAndBuildStringPlaceholder(const std::string& placeholder);
	SgVariableDeclaration* checkAndBuildPlaceholderForString(const std::string& string);
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
	void runTransformation(bool transformGlobals);
	void simplifyFunction(SgFunctionDeclaration *func, bool transformGlobals);
	//transformGlobals: flag to indicate whether to inline global const char * vars where the values they point to can be statically determined
	void transformUnmodifiedStringVars();
private:
	void transformUnmodifiedStringVars(SgFunctionDeclaration *func, SgInitializedName *initName);
};

#endif /* CODESIMPLIFIER_H_ */
