/*
 * basicProgmemTransformer.cpp
 *
 *  Created on: Mar 24, 2016
 *      Author: root
 */


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

std::set<SgVariableDeclaration *> getProgmemableVarDecls() {
	//TODO: return the actual global string literal declaration. not their placeholders

}
