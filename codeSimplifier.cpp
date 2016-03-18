/*
 * codeSimplifier.cpp
 *
 *  Created on: Mar 18, 2016
 *      Author: root
 */
#include "codeSimplifier.h"

void SimplifyFunctionDeclaration::visit(SgNode *node) {
	switch (node->variantT()) {
		case V_SgInitializedName:{
						 visit(isSgInitializedName(node));
					 }
					 break;
		case V_SgFunctionDefinition: {
						     visit(isSgFunctionDefinition(node));
					     }
					     break;
		case V_SgVarRefExp: {
					    visit(isSgVarRefExp(node));
				    }
				    break;
		case V_SgStringVal: {
					    visit(isSgStringVal(node));
				    }
	}
}
void SimplifyFunctionDeclaration::visit(SgFunctionDefinition *defn) {
	insertStringPlaceholderDecls(defn->get_body());
}

void SimplifyFunctionDeclaration::visit(SgInitializedName *initName){
	SgVariableDeclaration * varDecl = isSgVariableDeclaration(initName->get_declaration());
	if(varDecl == NULL) {
		return;
	}
	bool dropVarDecl = false;
	SgInitializer* initializer = initName->get_initializer();
	//Drop declaration of constant pointers to string literals
	if(isSgAssignInitializer(initializer)) {
		if(isSgStringVal(isSgAssignInitializer(initializer)->get_operand())){
			dropVarDecl = true;
		}
	}

	SgType *type = initName->get_type();
	//Convert char arrays which have never been modified to const char * pointers to string literals
	SgType *eleType = SageInterface::getElementType(type);
	if(isSgArrayType(type) && eleType != NULL && isSgTypeChar(eleType)) {
		if(aliasAnalysis->isUnmodifiedStringOrCharArray(func, initName)) {
			SgType *newType =  SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
			initName->set_type(newType);
			return;
		}
	}

	//Drop additional char * pointers if the values they point to can be statically determined
	if(SageInterface::isPointerType(type) && eleType != NULL && isSgTypeChar(eleType)) {
		dropVarDecl = true;
	}

	//Steps to drop the associated variable declaration
	if(dropVarDecl) {
		varsToReplace.insert(varID(initName));
		SageInterface::removeStatement(varDecl,false);
	}
}

void SimplifyFunctionDeclaration::visit(SgVarRefExp *var){
	if(varsToReplace.find(varID(var)) != varsToReplace.end()) {
		//There should only be 1 alias
		varID alias = *(aliasAnalysis->getAliasesForVariableAtNode(var, varID(var)).begin());
		SgVarRefExp *ref = SageBuilder::buildVarRefExp(alias.str(), func->get_scope());		
		SageInterface::replaceExpression(var, ref);

	}
}

void SimplifyFunctionDeclaration::visit(SgStringVal *strVal) {
	SgVariableDeclaration *placeholder = slPlaceholders[strVal->get_value()];
	SgVarRefExp *ref = SageBuilder::buildVarRefExp(placeholder);
	SageInterface::replaceExpression(strVal, ref);
}

void SimplifyFunctionDeclaration::insertStringPlaceholderDecls(SgScopeStatement *scope) {
	SgTreeCopy tCopy;
	for(auto& str: sla->getStringLiteralsInFunction(func)){
		SgVariableDeclaration *varDec = isSgVariableDeclaration(sla->getPlaceholderForStringLiteral(str)->copy(tCopy));
		varDec->set_scope(scope);
		SageInterface::prependStatement(varDec, scope);
	}
}

void SimplifyOriginalCode::visit(SgNode *node) {
	SgFunctionDeclaration *func = isSgFunctionDeclaration(node);
	if(func){
		SimplifyFunctionDeclaration funcHelper(aliasAnalysis, sla, func);
		funcHelper.traverse(func, preorder);
	}
}
