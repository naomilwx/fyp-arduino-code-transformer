/*
 * codeSimplifier.cpp
 *
 *  Created on: Mar 18, 2016
 *      Author: root
 */
#include "codeSimplifier.h"

using namespace FunctionAnalysisHelper;

std::string STRING_LITERAL_PLACEHOLDER_PREFIX = "f";

void SimplifyFunctionDeclaration::runTransformation() {
	transformVarDecls();
	transformAssignments();

	transformVarRefs();
	removeStringLiterals();
	insertStringPlaceholderDecls();
}

void SimplifyFunctionDeclaration::runTransformation(std::map<std::string, SgVariableDeclaration *> &placeholders){
	this->slPlaceholders = placeholders;
	transformVarDecls();
	transformAssignments();

	transformVarRefs();
	removeStringLiterals();

	for(auto &item: slPlaceholders) {
		if(placeholders.find(item.first) == placeholders.end()){
			placeholders[item.first] = item.second;
		}
	}
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
		SgInitializedName* iname = isSgInitializedName(initName);
		runVarDeclTransfromation(iname);
	}
}

void SimplifyFunctionDeclaration::transformVarRefs(){
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
	if(isVarExprToReplace(lhs)) {
		SgStatement *oldStmt = isSgStatement(op->get_parent());
		if(isSgFunctionCallExp(rhs)){
			SgFunctionCallExp *call = isSgFunctionCallExp(rhs);
			SgExprStatement * funcCallStmt = SageBuilder::buildFunctionCallStmt(call->getAssociatedFunctionSymbol()->get_name(),
					call->get_type(),
					call->get_args(),
					func->get_definition()->get_body());
			SageInterface::replaceStatement(oldStmt, funcCallStmt);
		} else {
			SageInterface::removeStatement(oldStmt,false);
		}
	}
}

SgExpression * SimplifyFunctionDeclaration::lookupAlias(varID alias) {
	std::string aliasStr = alias.str();
	if(isFunctionParamPlaceholder(aliasStr)) {
		SgInitializedName *initName = getFunctionParamForPlaceholder(func, aliasStr);
		return SageBuilder::buildVarRefExp(initName, func->get_scope());
	}
	if(isStringLiteralPlaceholder(aliasStr)) {
		return SageBuilder::buildVarRefExp(checkAndBuildStringPlaceholder(aliasStr));
	}
	return alias.toSgExpression();
}

void SimplifyFunctionDeclaration::replaceWithAlias(SgVarRefExp *var) {
	varID alias = *(aliasAnalysis->getAliasesForVariableAtNode(var, varID(var)).begin());
	while(varsToReplace.find(alias) != varsToReplace.end()) {
		alias = *(aliasAnalysis->getAliasesForVariableAtNode(var, alias).begin());
	}
	SgExpression *aliasExp = lookupAlias(alias);
	SgType *aType = aliasExp->get_type();
	SgType *varType = var->get_type();
	SgExpression *oldExp = var;
	int diff = 0;
	if(aType != NULL  && varType != NULL) {
		diff = getPointerLevel(varType) - getPointerLevel(aType);
		printf("level diff %d\n", diff);
	}
	while(diff > 0) {
		SgExpression* parent = isSgExpression(oldExp->get_parent());
		printf("parent: %s %s\n", parent->class_name().c_str(), parent->unparseToString().c_str());
		if(parent) {
			oldExp = parent;
		} else {
			break;
		}
		diff -= 1;
	}
	SageInterface::replaceExpression(oldExp, aliasExp);
}

bool SimplifyFunctionDeclaration::isVarExprToReplace(SgExpression *expr) {
	if(varID::isValidVarExp(expr) && varsToReplace.find(varID(expr)) != varsToReplace.end()){
		return true;
	}
	return false;
}

void SimplifyFunctionDeclaration::runVarRefsTransformation(SgVarRefExp *var) {
	printf("checking vars: %s\n", var->unparseToString().c_str());
//			if(var->isUsedAsLValue()) {
//				printf("l val: %s\n", var->unparseToString().c_str());
//			} else {
//				printf("r val: %s\n", var->unparseToString().c_str());
//			}
	if(isVarExprToReplace(var)) {
		//TODO: check for isUsedAsLValue = false and known alias instead...
		printf("replacing vars: %s\n", var->unparseToString().c_str());
		//TODO: need to consider alias type
		replaceWithAlias(var);
		printf("done replacing vars \n");

	}

}

void SimplifyFunctionDeclaration::replaceVarRefs(std::map<std::string, SgVariableDeclaration *>& placeholderMap, std::set<varID> vars) {
	this->varsToReplace = vars;
	this->slPlaceholders = placeholderMap;
	transformVarRefs();
	for(auto &item: slPlaceholders) {
		if(placeholderMap.find(item.first) == placeholderMap.end()){
			placeholderMap[item.first] = item.second;
		}
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
	SgVariableDeclaration *placeholder = checkAndBuildPlaceholderForString(strVal->get_value());
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

	SgType *type = initName->get_type();
	//Convert char arrays cannot be initialised with char* pointers
	SgType *eleType = SageInterface::getElementType(type);
	if(isSgArrayType(type) && eleType != NULL && isSgTypeChar(eleType)) {
		ignoredInitializers.insert(initializer);
		return;
	}
	
	//Drop additional char * pointers if the values they point to can be statically determined
	/*TODO: this is problematic when one of the function's param is a reassigned parameter eg:
	  void f(const char *&x) {
	  const char *orig_x = x;
	  x = "new str";
	  ...
	  }
	  orig_x will get wrongly subsituted for x. need to write a procedure to search for assignments to reference params...
	 */
	if(SageInterface::isPointerType(type) && isSgTypeChar(type->findBaseType())) {
		//TODO: this is problematic if there is an address on operation on the variable later on
		if(aliasAnalysis->isStaticallyDeterminatePointer(func, initName)){
			//Steps to drop the associated variable declaration
			SgExprStatement * funcCallStmt = NULL;

			varsToReplace.insert(varID(initName));
			if(isSgAssignInitializer(initializer)) {
				SgExpression *rhs = isSgAssignInitializer(initializer)->get_operand();
				if(isSgFunctionCallExp(rhs)) {
					SgFunctionCallExp *call = isSgFunctionCallExp(rhs);
					funcCallStmt = SageBuilder::buildFunctionCallStmt(call->getAssociatedFunctionSymbol()->get_name(),
							call->get_type(),
							call->get_args(),
							func->get_definition()->get_body());
				}
			}


			if(funcCallStmt != NULL) {
				SageInterface::replaceStatement(varDecl, funcCallStmt, true);
			} else {
				SageInterface::removeStatement(varDecl,false);
			}
			printf("removing var decl for: %s\n", initName->get_name().str());
		}
	}

}

void SimplifyFunctionDeclaration::insertStringPlaceholderDecls() {
	for(auto &item: slPlaceholders) {
		SageInterface::prependStatement(item.second, func->get_definition()->get_body());
	}
}

SgVariableDeclaration* SimplifyFunctionDeclaration::checkAndBuildStringPlaceholder(const std::string& placeholder){
	std::string str = sla->getStringLiteralForLabel(placeholder);
	if(slPlaceholders.find(str) == slPlaceholders.end()) {
		return buildStringPlaceholder(str, placeholder);
	}
	return slPlaceholders[str];
}

SgVariableDeclaration* SimplifyFunctionDeclaration::checkAndBuildPlaceholderForString(const std::string& str) {
	if(slPlaceholders.find(str) == slPlaceholders.end()) {
		std::string placeholder = sla->getStringLiteralLabel(str);
		return buildStringPlaceholder(str, placeholder);
	}
	return slPlaceholders[str];
}

SgVariableDeclaration* SimplifyFunctionDeclaration::buildStringPlaceholder(const std::string& str, const std::string& placeholder) {
	return SimplifyOriginalCode::buildStringPlaceholder(slPlaceholders, str, placeholder, varDeclsScope);
}


/**
 * SimplifyOriginalCode
 * */

SgVariableDeclaration* SimplifyOriginalCode::buildStringPlaceholder(std::map<std::string, SgVariableDeclaration *>& placeholderMap, const std::string& str, const std::string& placeholder, SgScopeStatement *scope) {
	SgType *type = SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
	SgAssignInitializer *initializer = SageBuilder::buildAssignInitializer(SageBuilder::buildStringVal(str));
	SgVariableDeclaration *varDec = SageBuilder::buildVariableDeclaration(STRING_LITERAL_PLACEHOLDER_PREFIX + placeholder, type, initializer, scope);
	placeholderMap[str] = varDec;
	return varDec;
}

void SimplifyOriginalCode::runTransformation() {
	for(auto &func: getDefinedFunctions(project)) {
		simplifyFunction(func);
	}
}

void SimplifyOriginalCode::transformGlobalVars() {
	std::vector<SgInitializedName *> globalVars = getGlobalVars(project);
	std::set<varID> varsToReplace;
	std::vector<SgInitializedName *>remainingGlobals;

	for(auto &var: globalVars) {
		printf("checking global var %s\n", var->get_name().str());
		SgType *type = var->get_type();

		if(!isSgArrayType(type) && SageInterface::isPointerType(type) && isConstantValueGlobalVar(var)) {
			SgInitializer* initializer = var->get_initializer();
			if(isSgAssignInitializer(initializer)) {
				SgExpression *rhs = isSgAssignInitializer(initializer)->get_operand();
				if(isSgStringVal(rhs)){
					printf("removing global var %s\n", var->get_name().str());
					SgVariableDeclaration * varDecl = isSgVariableDeclaration(var->get_declaration());
					SageInterface::removeStatement(varDecl, true);
					varsToReplace.insert(varID(var));
					continue;
				}
			}
		}
		remainingGlobals.push_back(var);
	}
	removeStringLiteralsInDecls(remainingGlobals);
	replaceGlobalVars(varsToReplace);
	printf("done transform global vars\n");
}

void SimplifyOriginalCode::insertPlaceholderDecls() {
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	SgStatement *firstDecl = SageInterface::getFirstStatement(global);
	for(auto &item: sharedPlaceholders) {
		SageInterface::insertStatement(firstDecl, item.second, true);
	}
}

void SimplifyOriginalCode::replaceGlobalVars(std::set<varID> vars) {
	if(vars.size() == 0){
		return;
	}
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	for(auto &func: getDefinedFunctions(project)) {
		printf("running replace global vars\n");
		SimplifyFunctionDeclaration funcHelper(aliasAnalysis, sla, func, project, global);
		funcHelper.replaceVarRefs(sharedPlaceholders, vars);
	}
}

bool SimplifyOriginalCode::isConstantValueGlobalVar(SgInitializedName* initName){
	bool isConstant = true;
	for(auto &func: getDefinedFunctions(project)) {
		if(!aliasAnalysis->isNotReassignedOrModified(func, initName)) {
			return false;
		}
	}
	return isConstant;
}

void SimplifyOriginalCode::runGlobalTransformation(){
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	transformGlobalVars();
	for(auto &func: getDefinedFunctions(project)) {
		simplifyFunction(func, global);
	}
	insertPlaceholderDecls();
}

void SimplifyOriginalCode::simplifyFunction(SgFunctionDeclaration *func, SgScopeStatement *varDeclScope) {
	SimplifyFunctionDeclaration funcHelper(aliasAnalysis, sla, func, project, varDeclScope);
	funcHelper.runTransformation(sharedPlaceholders);
	printf("function simp result:\n %s\n", func->unparseToString().c_str());
}

void SimplifyOriginalCode::simplifyFunction(SgFunctionDeclaration *func) {
	SimplifyFunctionDeclaration funcHelper(aliasAnalysis, sla, func, project);
	funcHelper.runTransformation();
	printf("function simp result:\n %s\n", func->unparseToString().c_str());
}

void SimplifyOriginalCode::transformUnmodifiedStringVars() {
	for(auto &func: getDefinedFunctions(project)) {
		Rose_STL_Container<SgNode *> initNames = NodeQuery::querySubTree(func, V_SgInitializedName);
		for(auto& initName: initNames) {
			SgInitializedName* iname = isSgInitializedName(initName);
			transformUnmodifiedStringVars(func, iname);
		}
	}
}

void SimplifyOriginalCode::transformUnmodifiedStringVars(SgFunctionDeclaration *func, SgInitializedName *initName) {
	SgType *type = initName->get_type();
	//Convert char arrays which have never been modified to const char * pointers to string literals
	SgType *eleType = SageInterface::getElementType(type);
	//	if(isArduinoStringType(type) || (isSgArrayType(type) && eleType != NULL && isSgTypeChar(eleType))) { //TODO: handle Arduino strings properly
	if(isSgArrayType(type) && eleType != NULL && isSgTypeChar(eleType)) {
		printf("checking %s\n", initName->unparseToString().c_str());
		if(aliasAnalysis->isUnmodifiedStringOrCharArray(func, initName)) {
			printf("setting type to const char *\n");
			SgType *newType =  SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
			initName->set_type(newType);
		}
	}
}

void  SimplifyOriginalCode::removeStringLiteralsInDecls(std::vector<SgInitializedName *> globalVars) {
	for(auto &initName: globalVars){
		Rose_STL_Container<SgNode *> stringLiterals = NodeQuery::querySubTree(initName, V_SgStringVal);
		for(auto &strLiteral: stringLiterals) {
			removeStringLiteral(isSgStringVal(strLiteral));
		}
	}
}

void  SimplifyOriginalCode::removeStringLiteral(SgStringVal *strVal) {
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	std::string label = sla->getStringLiteralLabel(strVal->get_value());
	SgVariableDeclaration *placeholder = buildStringPlaceholder(sharedPlaceholders, strVal->get_value(), label, global);
	SgVarRefExp *ref = SageBuilder::buildVarRefExp(placeholder);
	SageInterface::replaceExpression(strVal, ref);
}
