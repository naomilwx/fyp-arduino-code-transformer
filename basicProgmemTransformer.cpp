/*
 * basicProgmemTransformer.cpp
 *
 *  Created on: Mar 24, 2016
 *      Author: root
 */

#include "basicProgmemTransformer.h"

void BasicProgmemTransformer::runTransformation() {

}

int BasicProgmemTransformer::getBuffersizeNeededForFunction(SgFunctionDeclaration *func) {
	Rose_STL_Container<SgNode *> funcCalls = NodeQuery::querySubTree(func, V_SgFunctionCallExp);
	//TODO:
	return 0;
}

void BasicProgmemTransformer::shiftVarDeclsToProgmem() {

}

void BasicProgmemTransformer::transformFunction(SgFunctionDeclaration *func) {

}

std::set<varID> BasicProgmemTransformer::getVarsBoundToNonPlaceholderPointers() {
	std::set<varID> results;
	Rose_STL_Container<SgNode *> varRefs = NodeQuery::querySubTree(project, V_SgVarRefExp);
	for(auto &ref: varRefs) {
		if(aliasAnalysis->variableAtNodeHasKnownAlias(ref, varID(ref)) == false || isSgVarRefExp(ref)->isUsedAsLValue()) {
			std::set<varID> aliases = aliasAnalysis->getAliasesForVariableAtNode(ref, varID(ref));
			results.insert(aliases.begin(), aliases.end());
		}
	}
	return results;
}

std::set<varID> BasicProgmemTransformer::getVarsInUnsafeFunctionCalls() {
	std::set<varID> results;
	for(auto &func: getDefinedFunctions(project)){
		Rose_STL_Container<SgNode *> funcCalls = NodeQuery::querySubTree(func, V_SgFunctionCallExp);
		for(auto &fcall: funcCalls) {
			std::set<varID> vars = aliasAnalysis->getAliasesAtProgmemUnsafePositions(isSgFunctionCallExp(fcall));
			results.insert(vars.begin(), vars.end());
		}
	}
	return results;
}

std::set<varID> BasicProgmemTransformer::getVarsReturnedByFunctions() {
	std::set<varID> results;
	for(auto&func: getDefinedFunctions(project)) {
		std::set<varID> vals = aliasAnalysis->getPossibleReturnValues(func);
		results.insert(vals.begin(), vals.end());
	}
	return results;
}

std::set<varID> BasicProgmemTransformer::getProgmemablePlaceholders() {
	std::set<varID> placeholderIDs = sla->getPlaceholderVarIDs();
	std::set<varID> results;
	std::set<varID> varsInFuncRet = getVarsReturnedByFunctions();
	std::set<varID> varsInUnsafe = getVarsInUnsafeFunctionCalls();
	std::set<varID> varsBound = getVarsBoundToNonPlaceholderPointers();
	for(auto& var: placeholderIDs) {
		if(varsInFuncRet.find(var) == varsInFuncRet.end() && varsInUnsafe.find(var) == varsInUnsafe.end()) {
			if(varsBound.find(var) == varsBound.end()) {
				results.insert(var);
			}
		}
	}
	return results;
}

std::vector<SgVariableDeclaration *> BasicProgmemTransformer::getProgmemableVarDecls() {
	//TODO: return the actual global string literal declaration. not their placeholders
	std::vector<SgInitializedName *> globals = getGlobalVars(project);
	std::set<varID> safePlaceholders = getProgmemablePlaceholders();
	std::vector<SgVariableDeclaration *>results;
	for(auto& global:globals) {
		SgAssignInitializer* init = isSgAssignInitializer(global->get_initializer());
		if(init == NULL) {continue;}
		SgExpression *assigned = init->get_operand();
		if(isSgStringVal(assigned)) {
			varID placeholder = sla->getPlaceholderVarIDForStringLiteral(isSgStringVal(assigned)->get_value());
			if(safePlaceholders.find(placeholder) != safePlaceholders.end()) {
				SgVariableDeclaration *decl = isSgVariableDeclaration(global->get_declaration());
				if(decl) {
					results.push_back(decl);
				}
			}
		}
	}
	return results;
}
