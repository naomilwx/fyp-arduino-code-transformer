/*
 * codeSimplifier.cpp
 *
 *  Created on: Mar 18, 2016
 *      Author: root
 */
#include "codeSimplifier.h"

void SimplifyFunctionDeclaration::runTransformation() {
	transformGlobals();
	transformVarDecls();
	transformAssignments();

	buildStringPlaceholders();

	tranformVarRefs();
	removeStringLiterals();
	insertStringPlaceholderDecls();
}

void SimplifyFunctionDeclaration::transformAssignments() {
	//remove assignment statements for variable declarations that have been eliminated
	Rose_STL_Container<SgNode *> assignOps = NodeQuery::querySubTree(func, V_SgAssignOp);
		for(auto& assignOp: assignOps) {
			runAssignmentTransformation(isSgAssignOp(assignOp));
		}
}

void SimplifyFunctionDeclaration::transformVarDecls(){
	Rose_STL_Container<SgNode *> initNames = NodeQuery::querySubTree(func, V_SgInitializedName);
	for(auto& initName: initNames) {
		runVarDeclTransfromation(isSgInitializedName(initName));
	}
}

void SimplifyFunctionDeclaration::transformGlobals() {
	std::vector<SgInitializedName *> globalVars = getGlobalVars(project);
	for(auto &var: globalVars) {
		SgType *type = var->get_type();
		if(SageInterface::isPointerType(type) && isSgTypeChar(type->findBaseType())) {
			varsToReplace.insert(varID(var));
		}
	}
}

void SimplifyFunctionDeclaration::tranformVarRefs(){
	Rose_STL_Container<SgNode *> varRefs = NodeQuery::querySubTree(func, V_SgVarRefExp);
	for(auto &varRef: varRefs) {
		runVarRefsTransformation(isSgVarRefExp(varRef));
	}
}

void SimplifyFunctionDeclaration::removeStringLiterals() {
	Rose_STL_Container<SgNode *> stringLiterals = NodeQuery::querySubTree(func, V_SgStringVal);
	for(auto &strLiteral: stringLiterals) {
		runStringLiteralsTransformation(isSgStringVal(strLiteral));
	}
}

void SimplifyFunctionDeclaration::runAssignmentTransformation(SgAssignOp *op) {
	SgExpression *lhs = NULL;
	SgExpression *rhs = NULL;

	SageInterface::isAssignmentStatement(op,&lhs, &rhs);
	if(varID::isValidVarExp(lhs) && varsToReplace.find(varID(lhs)) != varsToReplace.end()) {
		SageInterface::removeStatement(isSgStatement(op->get_parent()), false);
	}
}

void SimplifyFunctionDeclaration::checkAlias(varID alias) {
	std::string aliasStr = alias.str();
	if(isStringLiteralPlaceholder(aliasStr)) {
		checkAndBuildStringPlaceholder(aliasStr);
	}
}

void SimplifyFunctionDeclaration::replaceWithAlias(SgVarRefExp *var) {
	varID alias = *(aliasAnalysis->getAliasesForVariableAtNode(var, varID(var)).begin());
	while(varsToReplace.find(alias) != varsToReplace.end()) {
		alias = *(aliasAnalysis->getAliasesForVariableAtNode(var, alias).begin());
	}
	SgVarRefExp *ref = SageBuilder::buildVarRefExp(alias.str(), func->get_scope());
	SgType *aType = alias.toSgExpression()->get_type();
	SgType *varType = var->get_type();
	SgExpression *oldExp = var;
	int diff = 0;
	if(aType != NULL  && varType != NULL) {
		diff = getPointerLevel(varType) - getPointerLevel(aType);
		printf("level diff %d\n", diff);
	}
	while(diff > 0) {
		SgExpression* parent = isSgExpression(oldExp->get_parent());
		if(parent) {
			oldExp = parent;
		} else {
			break;
		}
		diff -= 1;
	}
	SageInterface::replaceExpression(oldExp, ref);
}

bool SimplifyFunctionDeclaration::isVarExprToReplace(SgExpression *expr) {
	if(varID::isValidVarExp(expr) && varsToReplace.find(varID(expr)) != varsToReplace.end()){
		return true;
	}
	return false;
}

void SimplifyFunctionDeclaration::runVarRefsTransformation(SgVarRefExp *var) {
	printf("checking vars: %s\n", var->unparseToString().c_str());
	if(isVarExprToReplace(var)) {
		printf("replacing vars: %s\n", var->unparseToString().c_str());
		//TODO: need to consider alias type
		replaceWithAlias(var);
		printf("done replacing vars \n");

	}

}

void SimplifyFunctionDeclaration::runStringLiteralsTransformation(SgStringVal *strVal){
	SgNode *parent = strVal->get_parent();
	if(isSgInitializer(parent)) {
		//Skip replacement if this is part of a char array initialisation
		if(ignoredInitializers.find(isSgInitializer(parent)) != ignoredInitializers.end()){
			return;
		}
	}
	printf("replacing string literal: %s %p\n", strVal->get_value().c_str(), strVal);
	SgVariableDeclaration *placeholder = slPlaceholders[strVal->get_value()];
	SgVarRefExp *ref = SageBuilder::buildVarRefExp(placeholder);
	SageInterface::replaceExpression(strVal, ref);
	printf("done replacing string literal\n");
}

void SimplifyFunctionDeclaration::runVarDeclTransfromation(SgInitializedName *initName) {
	SgVariableDeclaration * varDecl = isSgVariableDeclaration(initName->get_declaration());
	if(varDecl == NULL) {
		return;
	}
	printf("checking var decl: %s\n", initName->unparseToString().c_str());
	SgInitializer* initializer = initName->get_initializer();
	bool dropVarDecl = false;

	SgType *type = initName->get_type();
	//Convert char arrays which have never been modified to const char * pointers to string literals
	SgType *eleType = SageInterface::getElementType(type);
	if(isSgArrayType(type) && eleType != NULL && isSgTypeChar(eleType)) {
		ignoredInitializers.insert(initializer);
		if(aliasAnalysis->isUnmodifiedStringOrCharArray(func, initName)) {
			printf("setting type to const char *\n");
			SgType *newType =  SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
			initName->set_type(newType);
		}
		return;
	}


	//Drop declaration of constant pointers to string literals
	if(isSgAssignInitializer(initializer)) {
		if(isSgStringVal(isSgAssignInitializer(initializer)->get_operand())){
			dropVarDecl = true;
			printf("marked 1\n");
		}
	}

	//Drop additional char * pointers if the values they point to can be statically determined
	if(SageInterface::isPointerType(type) && isSgTypeChar(type->findBaseType())) {
		if(aliasAnalysis->isMultiAssignmentPointer(func, initName) == false){
			dropVarDecl = true;
			printf("marked 2\n");
		} else {
			printf("unmarked 2\n");
			dropVarDecl = false;
		}
	}

	//Steps to drop the associated variable declaration
	if(dropVarDecl) {
		varsToReplace.insert(varID(initName));
		SageInterface::removeStatement(varDecl,false);
		printf("removing var decl for: %s\n", initName->get_name().str());
	}
}

void SimplifyFunctionDeclaration::insertStringPlaceholderDecls() {
	for(auto &item: slPlaceholders) {
		SageInterface::prependStatement(item.second, func->get_definition()->get_body());
	}
}

void SimplifyFunctionDeclaration::checkAndBuildStringPlaceholder(const std::string placeholder){
	if(builtPlaceholders.find(placeholder) == builtPlaceholders.end()) {
		std::string str = sla->getStringLiteralForLabel(placeholder);
		buildStringPlaceholder(str, placeholder);
	}
}

void SimplifyFunctionDeclaration::buildStringPlaceholders(){
	for(auto& str: sla->getStringLiteralsInFunction(func)){
		std::string pName = sla->getStringLiteralLabel(str);
		buildStringPlaceholder(str, pName);
	}
}

void SimplifyFunctionDeclaration::buildStringPlaceholder(const std::string& str, const std::string& placeholder) {
	SgType *type = SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
	SgScopeStatement *scope = func->get_definition()->get_body();
	SgAssignInitializer *initializer = SageBuilder::buildAssignInitializer(SageBuilder::buildStringVal(str));
	SgVariableDeclaration *varDec = SageBuilder::buildVariableDeclaration(placeholder, type, initializer, scope);
	slPlaceholders[str] = varDec;
	builtPlaceholders.insert(placeholder);
}

void SimplifyOriginalCode::runTransformation() {
	for(auto &func: getDefinedFunctions(project)) {
		simplifyFunction(func);
	}
}

void SimplifyOriginalCode::simplifyFunction(SgFunctionDeclaration *func) {
	SimplifyFunctionDeclaration funcHelper(aliasAnalysis, sla, func, project);
	funcHelper.runTransformation();
	printf("function simp result:\n %s\n", func->unparseToString().c_str());
}
