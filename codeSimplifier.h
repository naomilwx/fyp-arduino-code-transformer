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

class SimplifyFunctionDeclaration : public AstSimpleProcessing {
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
	void visit(SgNode *node);
	void visit(SgFunctionDefinition *defn);
	void visit(SgInitializedName *initName);
	void visit(SgVarRefExp *var);
	void visit(SgStringVal *strVal);
private:
	std::map<std::string, SgVariableDeclaration *> slPlaceholders;
	std::set<varID> varsToReplace;

	void insertStringPlaceholderDecls(SgScopeStatement *scope);
};

class SimplifyOriginalCode: public AstSimpleProcessing {
protected:
	PointerAliasAnalysis *aliasAnalysis;
	StringLiteralAnalysis *sla;
public:
	void visit(SgNode *node);
};

#endif /* CODESIMPLIFIER_H_ */
