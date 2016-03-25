/*
 * basicProgmemTransformer.cpp
 *
 *  Created on: Mar 24, 2016
 *      Author: root
 */

#include <map>

#include "variables.h"
#include "convertibleFunctions.h"
#include "basicProgmemTransform.h"

void BasicProgmemTransform::runTransformation() {
	setupProgmemableVarDecls();

}

int BasicProgmemTransform::getBuffersizeNeededForFunction(SgFunctionDeclaration *func) {
	int maxSize = 0;
	Rose_STL_Container<SgNode *> funcCalls = NodeQuery::querySubTree(func, V_SgFunctionCallExp);
	//TODO:
	for(auto &funcCall: funcCalls) {
		SgFunctionCallExp *fcall = isSgFunctionCallExp(funcCall);
		Function callee(fcall);
		printf("function called: %s\n", callee.get_name().str());
		SgExpressionPtrList params = fcall->get_args()->get_expressions();
	}
	return maxSize;
}

void BasicProgmemTransform::shiftVarDeclsToProgmem() {

}

void BasicProgmemTransform::setupCharBufferForFunction(SgFunctionDeclaration *func) {

}

void BasicProgmemTransform::transformFunction(SgFunctionDeclaration *func) {

}

std::set<varID> BasicProgmemTransform::getVarsBoundToNonPlaceholderPointers() {
	std::set<varID> results;
	Rose_STL_Container<SgNode *> varRefs = NodeQuery::querySubTree(project, V_SgVarRefExp);
	for(auto &ref: varRefs) {
		SgVarRefExp *var = isSgVarRefExp(ref);
		if(isFromLibrary(var->get_symbol()->get_declaration())){
			continue;
		}

		printf("checking %s\n", ref->unparseToString().c_str());
		varID varRef = SgExpr2Var(var);
		if(aliasAnalysis->variableAtNodeHasKnownAlias(var, varRef) == false || var->isUsedAsLValue()) {
			printf("ref %s\n", ref->unparseToString().c_str());
			std::set<varID> aliases = aliasAnalysis->getAliasesForVariableAtNode(var, varRef);
			if(aliases.size() == 0) {
				aliases.insert(varRef);
			}
			results.insert(aliases.begin(), aliases.end());
		}
	}
	return results;
}

std::set<varID> BasicProgmemTransform::getVarsInUnsafeFunctionCalls() {
	std::set<varID> results;
	for(SgFunctionDeclaration *func: getDefinedFunctions(project)){
		Rose_STL_Container<SgNode *> funcCalls = NodeQuery::querySubTree(func, V_SgFunctionCallExp);
		for(auto &fcall: funcCalls) {
//			printf("func call %s\n", fcall->unparseToString().c_str());
			std::set<varID> vars = aliasAnalysis->getAliasesAtProgmemUnsafePositions(isSgFunctionCallExp(fcall));
			results.insert(vars.begin(), vars.end());
		}
	}
	return results;
}

std::set<varID> BasicProgmemTransform::getVarsReturnedByFunctions() {
	std::set<varID> results;
	for(SgFunctionDeclaration *func: getDefinedFunctions(project)) {
		std::set<varID> vals = aliasAnalysis->getPossibleReturnValues(func);
		results.insert(vals.begin(), vals.end());
	}
	return results;
}

std::set<varID> BasicProgmemTransform::getProgmemablePlaceholders() {
	std::set<varID> placeholderIDs = sla->getPlaceholderVarIDs();
	std::set<varID> results;
//	printf("before getting info...\n");
	std::set<varID> varsInFuncRet = getVarsReturnedByFunctions();
//	printf("done first..\n");
	std::set<varID> varsInUnsafe = getVarsInUnsafeFunctionCalls();
//	printf("done second..\n");
	std::set<varID> varsBound = getVarsBoundToNonPlaceholderPointers();
//	printf("done third..\n");
	for(auto& var: placeholderIDs) {
		if(varsInFuncRet.find(var) == varsInFuncRet.end() && varsInUnsafe.find(var) == varsInUnsafe.end()) {
			if(varsBound.find(var) == varsBound.end()) {
				results.insert(var);
			}
		}
	}
	return results;
}

void BasicProgmemTransform::setupProgmemableVarDecls() {
	std::vector<SgInitializedName *> globals = getGlobalVars(project);
	std::set<varID> safePlaceholders = getProgmemablePlaceholders();
	printf("getting globals...\n");
	for(auto& global:globals) {
		SgAssignInitializer* init = isSgAssignInitializer(global->get_initializer());
		if(init == NULL) {continue;}
		printf("checking.. %s\n", init->unparseToString().c_str());
		SgExpression *assigned = init->get_operand();
		if(isSgStringVal(assigned)) {
			varID placeholder = sla->getPlaceholderVarIDForStringLiteral(isSgStringVal(assigned)->get_value());
			if(safePlaceholders.find(placeholder) != safePlaceholders.end()) {
				SgVariableDeclaration *decl = isSgVariableDeclaration(global->get_declaration());
				if(decl) {
					printf("shifting %s\n", decl->unparseToString().c_str());
					varDeclsToShift.insert(decl);
				}
			}
		}
	}
	return;
}
