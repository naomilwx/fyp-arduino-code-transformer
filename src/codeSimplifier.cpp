/*
 * codeSimplifier.cpp
 *
 *  Created on: Mar 18, 2016
 *      Author: root
 */
#include <algorithm>

#include "codeSimplifier.h"
//#include "defUseChains.h"
#include "defsAndUsesUnfilteredCfg.h"

using namespace FunctionAnalysisHelper;

std::string STRING_LITERAL_PLACEHOLDER_PREFIX = "f";

void SimplifyFunctionDeclaration::runTransformation() {
	markArrayInitializers();

	transformVarRefs();
	removeStringLiterals();
	insertStringPlaceholderDecls();
}

void SimplifyFunctionDeclaration::runTransformation(std::map<std::string, SgVariableDeclaration *> &placeholders, std::map<SgNode*, std::set<SgNode*> > &defUseInfo){
	this->slPlaceholders = placeholders;
	this->defUseInfo = defUseInfo;
	markArrayInitializers();

	transformVarRefs();
	pruneUnusedVarDefinitions();
	pruneUnusedVarDeclarations();
	removeStringLiterals();

	for(auto &item: slPlaceholders) {
		if(placeholders.find(item.first) == placeholders.end()){
			placeholders[item.first] = item.second;
		}
	}
}

void SimplifyFunctionDeclaration::pruneUnusedVarDeclarations() {
	Rose_STL_Container<SgNode *> initNames = NodeQuery::querySubTree(func, V_SgInitializedName);
	Rose_STL_Container<SgNode *> varRefs = NodeQuery::querySubTree(func, V_SgVarRefExp);
	std::reverse(initNames.begin(), initNames.end());
	for(auto& init: initNames) {
		SgInitializedName* initName = isSgInitializedName(init);
//		if(initName->get_initializer() != NULL) { continue; }
		printf("checking var... %s\n", initName->unparseToString().c_str());
		bool foundRef = false;
		for(auto& ref:varRefs) {
			SgVarRefExp *var = isSgVarRefExp(ref);
			if((removedVarRefs.find(var) == removedVarRefs.end()) && (var->get_symbol()->get_declaration() == initName)) {
				foundRef = true;
				continue;
			}
		}
		if(foundRef == false) {
			removeVarDecl(initName);
			printf("removing stray... %s\n", initName->unparseToString().c_str());
		}
	}

}

void SimplifyFunctionDeclaration::pruneUnusedVarDefinitions() {
	printf("pruning\n");
	Rose_STL_Container<SgNode *> assignOps = NodeQuery::querySubTree(func, V_SgAssignOp);
	for(auto& assign: assignOps) {
		printf("%s\n", assign->unparseToString().c_str());
		SgAssignOp *op = isSgAssignOp(assign);
		SgExpression *lhs = op->get_lhs_operand();
		if(isSgUnaryOp(lhs)) { lhs = isSgUnaryOp(lhs)->get_operand();}
		if(!isSgVarRefExp(lhs)) { continue;} //Ignore Pointer/Address of/ Array index refs
		if(isGlobalVarRef(project, isSgVarRefExp(lhs))) { continue; }
		std::set<SgNode*> refs = defUseInfo[assign];
		bool redundant = true;
		if(SageInterface::isReferenceType(lhs->get_type())) {
			//this is a reference assignment x = ...; where x is a reference type
			continue;
		}
		for(auto& ref: refs) {
			SgVarRefExp *varRef = isSgVarRefExp(ref);
			if(varRef == NULL) { continue; }
			printf("usage: %s %d\n", ref->unparseToString().c_str(), ref->get_file_info()->get_line());
			if(removedVarRefs.find(varRef) == removedVarRefs.end()) {
				redundant = false; //found unremoved usage
				break;
			}
		}
		if(redundant) {
			printf("removing... %s\n", assign->unparseToString().c_str());
			removeVarAssignment(op);
		}
	}

	printf("pruning var decls \n");
	Rose_STL_Container<SgNode *> initNames = NodeQuery::querySubTree(func, V_SgInitializedName);
	for(auto &item: initNames) {
		SgInitializedName *initName = isSgInitializedName(item);
		printf("%s\n", initName->unparseToString().c_str());
		std::set<SgNode *> refs = defUseInfo[initName];
		bool redundant = true;
		for(auto& ref: refs) {
			SgVarRefExp *varRef = isSgVarRefExp(ref);
			if(varRef == NULL) { continue; }
			printf("usage: [%s] %s %d\n", ref->unparseToString().c_str(), ref->class_name().c_str(), ref->get_file_info()->get_line());
			if(removedVarRefs.find(varRef) == removedVarRefs.end()) {
				redundant = false;
				break;
			}
		}
		if(redundant) {
			printf("removing... %s\n", initName->unparseToString().c_str());
			removeInitializer(initName);
		}
	}
}

void SimplifyFunctionDeclaration::markArrayInitializers(){
	Rose_STL_Container<SgNode *> initNames = NodeQuery::querySubTree(func, V_SgInitializedName);
	for(auto& initName: initNames) {
		SgInitializedName* iname = isSgInitializedName(initName);
		markCharArrayInitializers(iname);
	}
}

bool SimplifyFunctionDeclaration::isReplacableVarRef(SgVarRefExp* varRef) {
	if(varRef == NULL) {
		return false;
	}
	std::string varName = varRef->get_symbol()->get_name().getString();
	std::string placeholderPref = STRING_LITERAL_PLACEHOLDER_PREFIX + STRING_LITERAL_PREFIX;
	if(varName.substr(0, placeholderPref.length()) == placeholderPref) {
		return false;
	}
	//TODO: instead of just excluding LValues, which is always safe,
	// consided allowing cases like arr[i] for arr to be replaced by its alias if it is a pointer to an array.
	// It's ok to replace as long as it is not inside an addressof or ref operator
	if((aliasAnalysis->variableAtNodeHasKnownAlias(varRef, SgExpr2Var(varRef)))== false) {
		return false;
	}
	SgNode *parent = varRef->get_parent();
	return (varRef->isUsedAsLValue() == false) || (isSgPntrArrRefExp(parent) || isSgPointerDerefExp(parent)); //Ok to replace with alias in the case of a pointer p pointing to an array in the expression p[]
}
void SimplifyFunctionDeclaration::transformVarRefs(){
	Rose_STL_Container<SgNode *> varRefs = NodeQuery::querySubTree(func, V_SgVarRefExp);
	for(auto &varRef: varRefs) {
		SgVarRefExp *var = isSgVarRefExp(varRef);
		printf("checking vars: %s\n", var->unparseToString().c_str());
		if(varID::isValidVarExp(var) == false) {
			continue;
		}
		if(isReplacableVarRef(var)) {
			//TODO: check for isUsedAsLValue = false and known alias instead...
			printf("replacing vars: %s %d\n", var->unparseToString().c_str(), var->get_file_info()->get_line());
			replaceWithAlias(var);
			printf("done replacing vars \n");

		}
	}
}


void SimplifyFunctionDeclaration::transformVarRefs(std::set<varID> varsToReplace){
	Rose_STL_Container<SgNode *> varRefs = NodeQuery::querySubTree(func, V_SgVarRefExp);
	for(auto &varRef: varRefs) {
		SgVarRefExp *var = isSgVarRefExp(varRef);
		printf("checking vars: %s\n", var->unparseToString().c_str());
		if(varID::isValidVarExp(var) && varsToReplace.find(varID(var)) != varsToReplace.end()) {
			printf("replacing vars: %s\n", var->unparseToString().c_str());
			replaceWithAlias(var);
			printf("done replacing vars \n");

		}
	}
}

void SimplifyFunctionDeclaration::removeStringLiterals() {
	Rose_STL_Container<SgNode *> stringLiterals = NodeQuery::querySubTree(func, V_SgStringVal);
	for(auto &strLiteral: stringLiterals) {
		runStringLiteralsTransformation(isSgStringVal(strLiteral));
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

bool SimplifyFunctionDeclaration::isTemporaryAlias(varID alias) {
	if(alias.str().substr(0, PointerAliasAnalysis::newExpPlaceholder.length()) == PointerAliasAnalysis::newExpPlaceholder) {
		return true;
	}
	return false;
}

void SimplifyFunctionDeclaration::replaceWithAlias(SgVarRefExp *var) {
	varID alias = *(aliasAnalysis->getAliasesForVariableAtNode(var, varID(var)).begin());
	if(isTemporaryAlias(alias)) {
		return;
	}
	SgExpression *aliasExp = lookupAlias(alias);
	SgType *aType = aliasExp->get_type();
	SgType *varType = var->get_type();
	SgExpression *oldExp = var;
	int diff = 0;
	if(aType != NULL  && varType != NULL) {
		diff = getPointerLevel(varType) - getPointerLevel(aType);
	}

	while(diff > 0) {
		SgExpression *par = isSgExpression(oldExp->get_parent());
		if(isSgAddressOfOp(par)) {
			break;
		}
		diff -= 1;
		if(par && (isSgPointerDerefExp(par) || isSgPntrArrRefExp(par))) {
			oldExp = par;
		} else {
			aliasExp = SageBuilder::buildAddressOfOp(aliasExp); //TODO: check correctness of this
			break;
		}
	}
	if(diff != 0) {
		return;
	}
	removedVarRefs.insert(var);
	printf("replace with %s\n", aliasExp->unparseToString().c_str());
	SageInterface::replaceExpression(oldExp, aliasExp, true);
}


void SimplifyFunctionDeclaration::replaceVarRefs(std::map<std::string, SgVariableDeclaration *>& placeholderMap, std::set<varID> vars) {
	this->slPlaceholders = placeholderMap;
	transformVarRefs(vars);
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


void SimplifyFunctionDeclaration::markCharArrayInitializers(SgInitializedName *initName) {
	SgVariableDeclaration * varDecl = isSgVariableDeclaration(initName->get_declaration());
	if(varDecl == NULL) {
		return;
	}
	SgInitializer* initializer = initName->get_initializer();
	SgType *type = initName->get_type();
	//Convert char arrays cannot be initialised with char* pointers

	if(isCharArrayType(type)) {
		ignoredInitializers.insert(initializer);
		printf("Marking arr: %s\n", initName->unparseToString().c_str());
		return;
	}

}

void SimplifyFunctionDeclaration::removeVarDecl(SgInitializedName *initName) {
	SgVariableDeclaration * varDecl = isSgVariableDeclaration(initName->get_declaration());
	if(varDecl == NULL) {
		return;
	}

	printf("removing var decl: %s\n", initName->unparseToString().c_str());
	SgInitializer* initializer = initName->get_initializer();

	SgExprStatement * funcCallStmt = NULL;

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
	printf("removed var decl for: %s\n", initName->get_name().str());
	markContainedVarRefsAsRemoved(varDecl);
}

void SimplifyFunctionDeclaration::removeInitializer(SgInitializedName *initName) {
	SgVariableDeclaration * varDecl = isSgVariableDeclaration(initName->get_declaration());
	if(varDecl == NULL) {
		return;
	}
	SgInitializer* initializer = initName->get_initializer();

		SgExprStatement * funcCallStmt = NULL;

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

//	SgVariableDeclartion *newDecl = SageBuilder::buildVariableDefinition_nfi(varDecl, initName, NULL);
//	SageInterface::replaceStatement(varDecl, newDecl, true);
	varDecl->reset_initializer(NULL);
	if(funcCallStmt != NULL) {
		SageInterface::insertStatementAfter(varDecl, funcCallStmt);
	}
	printf("removed var initializer for: %s\n", initName->get_name().str());
//	markContainedVarRefsAsRemoved(initializer);
}

void SimplifyFunctionDeclaration::removeVarAssignment(SgAssignOp *op) {
	SgExpression *lhs = NULL;
	SgExpression *rhs = NULL;

		SageInterface::isAssignmentStatement(op,&lhs, &rhs);
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
			markContainedVarRefsAsRemoved(oldStmt);
}

void SimplifyFunctionDeclaration::markContainedVarRefsAsRemoved(SgNode *node) {
	Rose_STL_Container<SgNode *> varRefs = NodeQuery::querySubTree(node, V_SgVarRefExp);
	for(auto& var: varRefs) {
		removedVarRefs.insert(isSgVarRefExp(var));
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

SimplifyOriginalCode::SimplifyOriginalCode(PointerAliasAnalysis *a, StringLiteralAnalysis* s, SgProject *p){
		this->aliasAnalysis = a;
		this->sla = s;
		this->project = p;

};

SgVariableDeclaration* SimplifyOriginalCode::buildStringPlaceholder(std::map<std::string, SgVariableDeclaration *>& placeholderMap, const std::string& str, const std::string& placeholder, SgScopeStatement *scope) {
	SgType *type = SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
	SgAssignInitializer *initializer = SageBuilder::buildAssignInitializer(SageBuilder::buildStringVal(str));
	SgVariableDeclaration *varDec = SageBuilder::buildVariableDeclaration(STRING_LITERAL_PLACEHOLDER_PREFIX + placeholder, type, initializer, scope);
	placeholderMap[str] = varDec;
	return varDec;
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

void SimplifyOriginalCode::runGlobalTransformation(std::map<SgNode*, std::set<SgNode*> > &defUse){
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	transformGlobalVars();
	for(auto &func: getDefinedFunctions(project)) {
		simplifyFunction(func, global, defUse);
	}
	insertPlaceholderDecls();
}

void SimplifyOriginalCode::simplifyFunction(SgFunctionDeclaration *func, SgScopeStatement *varDeclScope, std::map<SgNode*, std::set<SgNode*> > &defUseInfo) {
	SimplifyFunctionDeclaration funcHelper(aliasAnalysis, sla, func, project, varDeclScope);
	funcHelper.runTransformation(sharedPlaceholders, defUseInfo);
	printf("function simp result:\n %s\n", func->unparseToString().c_str());
}

void SimplifyOriginalCode::transformUnmodifiedStringVars() {
	for(auto &func: getDefinedFunctions(project)) {
		Rose_STL_Container<SgNode *> initNames = NodeQuery::querySubTree(func, V_SgInitializedName);
		for(auto& initName: initNames) {
			SgInitializedName* iname = isSgInitializedName(initName);
			transformUnmodifiedStringVars(func, iname);
		}

		//change unmodified args to const for const correctness req of frontend
		std::set<int> unmodPos = aliasAnalysis->getUnModifiedPositions(func);
		SgInitializedNamePtrList args = func->get_args();
		for(int pos: unmodPos) {
			SgInitializedName *arg = args[pos];
			SgType *type = arg->get_type();
			if(SageInterface::isPointerToNonConstType(type) == false) {
				continue;
			}
			if(isCharPointerType(type) || isCharArrayType(type)) {
				SgType *newType =  SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
				arg->set_type(newType);
			}
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
		if(isCharArrayType(initName->get_type())) {
			continue;
		}
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

void SimplifyOriginalCode::simplifyFunction(SgFunctionDeclaration *func) {
	SimplifyFunctionDeclaration funcHelper(aliasAnalysis, sla, func, project);
	funcHelper.runTransformation();
	printf("function simp result:\n %s\n", func->unparseToString().c_str());
}
