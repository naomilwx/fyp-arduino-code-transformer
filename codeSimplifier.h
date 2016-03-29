/*
 * codeSimplifier.h
 *
 *  Created on: Mar 18, 2016
 *      Author: root
 */

#ifndef CODESIMPLIFIER_H_
#define CODESIMPLIFIER_H_

#include <map>

#include "rose.h"
#include "stringValPropagation.h"

class SimplifyFunctionDeclaration {
protected:
	PointerAliasAnalysis *aliasAnalysis;
	StringLiteralAnalysis *sla;
	SgFunctionDeclaration *func;
	SgProject *project;
	SgScopeStatement *varDeclsScope;

	std::map<SgNode*, std::set<SgNode*> > defUseInfo;
public:
	SimplifyFunctionDeclaration(PointerAliasAnalysis *a, StringLiteralAnalysis *s, SgFunctionDeclaration *f, SgProject *p){
		this->aliasAnalysis = a;
		this->sla = s;
		this->func = f;
		this->project = p;
		this->varDeclsScope = func->get_definition()->get_body();
	}

	SimplifyFunctionDeclaration(PointerAliasAnalysis *a, StringLiteralAnalysis *s, SgFunctionDeclaration *f, SgProject *p, SgScopeStatement*scope){
		this->aliasAnalysis = a;
				this->sla = s;
				this->func = f;
				this->project = p;
				this->varDeclsScope = scope;
	}

	void replaceVarRefs(std::map<std::string, SgVariableDeclaration *>& placeholderMap, std::set<varID> vars); //Method to replace vars in list with their alias. Assumes that the vars are not reassigned.


	void removeStringLiterals();

	/**
	 * Unify string literals: for create a single const char * pointer for each unique String Literal
	 * Finds all occurances of the string literals and replace them with their respective pointer
	 * Collaspes char * pointers which point to the same item in memory into a single pointer
	 *
	 * */
	void runTransformation();
	/**
	 * slPlaceholders: Global scope place holder variables to be used to replace the string literals
	 * */
	void runTransformation(std::map<std::string, SgVariableDeclaration *> &slPlaceholders, std::map<SgNode*, std::set<SgNode*> > &defUseInfo);

private:
	//map of string literal to the placeholder for the string
	std::map<std::string, SgVariableDeclaration *> slPlaceholders;
	std::set<SgInitializer *> ignoredInitializers;
	std::set<SgVarRefExp*> removedVarRefs;

	void pruneUnusedVarDefinitions();
	void pruneUnusedVarDeclarations(); //Remove variable declarations that are now left unused

	void transformVarRefs();
	void transformVarRefs(std::set<varID> varsToReplace);

	void removeVarDecl(SgInitializedName *name);
	void removeVarAssignment(SgAssignOp *op);
	void removeInitializer(SgInitializedName *name);
	void markContainedVarRefsAsRemoved(SgNode *node);

	bool isReplacableVarRef(SgVarRefExp *varRef);
//	void runVarRefsTransformation(SgVarRefExp *var);
	void runStringLiteralsTransformation(SgStringVal *strVal);

	void markArrayInitializers();
	void markCharArrayInitializers(SgInitializedName *initName);

	void insertStringPlaceholderDecls();

	SgExpression * lookupAlias(varID alias);
	void replaceWithAlias(SgVarRefExp *var);

	SgVariableDeclaration* checkAndBuildStringPlaceholder(const std::string& placeholder);
	SgVariableDeclaration* checkAndBuildPlaceholderForString(const std::string& string);
	SgVariableDeclaration* buildStringPlaceholder(const std::string& str, const std::string& placeholder);


};

class SimplifyOriginalCode {
protected:
	PointerAliasAnalysis *aliasAnalysis;
	StringLiteralAnalysis *sla;
	SgProject *project;
public:
	SimplifyOriginalCode(PointerAliasAnalysis *a, StringLiteralAnalysis* s, SgProject *p);

	static SgVariableDeclaration* buildStringPlaceholder(std::map<std::string, SgVariableDeclaration *>& slPlaceholders, const std::string& str, const std::string& placeholder, SgScopeStatement *scope);

	void runGlobalTransformation(std::map<SgNode*, std::set<SgNode*> > & defUse);
//	void runTransformation();

	void transformGlobalVars();
	void simplifyFunction(SgFunctionDeclaration *func);
	void simplifyFunction(SgFunctionDeclaration *func, SgScopeStatement *varDeclScope, std::map<SgNode*, std::set<SgNode*> > &defUseInfo);

	void transformUnmodifiedStringVars();

private:
	std::map<std::string, SgVariableDeclaration *> sharedPlaceholders;
	void removeStringLiteralsInDecls(std::vector<SgInitializedName *> globalVars);
	void removeStringLiteral(SgStringVal *strVal);
	void replaceGlobalVars(std::set<varID> vars);
	bool isConstantValueGlobalVar(SgInitializedName *initName);
	void transformUnmodifiedStringVars(SgFunctionDeclaration *func, SgInitializedName *initName);
	void insertPlaceholderDecls();
};

#endif /* CODESIMPLIFIER_H_ */
