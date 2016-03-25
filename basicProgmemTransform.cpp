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

using namespace ConvertibleFunctions;

std::string FUNC_BUFFER_NAME = "f_arrbuf";
void BasicProgmemTransform::runTransformation() {
	setupProgmemableVarDecls();
	for(auto &func: getDefinedFunctions(project)) {
		transformFunction(func);
	}
	shiftVarDeclsToProgmem();
}

int BasicProgmemTransform::getBuffersizeNeededForFunction(SgFunctionDeclaration *func) {
	int maxSize = 0;
	Rose_STL_Container<SgNode *> funcCalls = NodeQuery::querySubTree(func, V_SgFunctionCallExp);
	for(auto &funcCall: funcCalls) {
		SgFunctionCallExp *fcall = isSgFunctionCallExp(funcCall);
		Function callee(fcall);
//		printf("function called: %s\n", callee.get_name().str());
		if(isArduinoProgmemSafeFunction(callee)) {
			continue;
		}
		param_pos_list ignoredPositions = getPositionsToIgnore(callee.get_name().getString());
		SgExpressionPtrList params = fcall->get_args()->get_expressions();
		int size = 0;
		for(int pos = 0; pos < params.size(); pos++) {
			if(ignoredPositions.find(pos) != ignoredPositions.end()) {
				continue;
			}
			SgVarRefExp* var = isSgVarRefExp(params[pos]);
			if(var) {
				SgInitializedName *initName = var->get_symbol()->get_declaration();
				if(isVarDeclToRemove(initName)) {
					SgExpression *rhs = getRHSOfVarDecl(initName);
					if(rhs && isSgStringVal(rhs)) {
						size += isSgStringVal(rhs)->get_value().size() + 1;
					}
				}
			}
		}
		if(size > maxSize) {
			maxSize = size;
		}
	}
	return maxSize;
}

int BasicProgmemTransform::getSizeNeededToLoadFromProgmem(SgVarRefExp *var) {
	SgInitializedName *initName = var->get_symbol()->get_declaration();
	SgExpression *rhs = getRHSOfVarDecl(initName);
	if(rhs && isSgStringVal(rhs)) {
		return isSgStringVal(rhs)->get_value().size() + 1;
	}
	return 0;
}

void BasicProgmemTransform::shiftVarDeclsToProgmem() {
	std::string flashHelper = "#define FS(x)(__FlashStringHelper*)(x)";
	insertPreprocessingInfo(flashHelper);
	for(auto& varDecl : varDeclsToShift) {
		convertVarDeclToProgmemDecl(varDecl);
	}
}
void BasicProgmemTransform::insertPreprocessingInfo(const std::string &data) {
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	SgStatement *firstDecl = SageInterface::getFirstStatement(global);
	PreprocessingInfo* result = new PreprocessingInfo(PreprocessingInfo::CpreprocessorIncludeDeclaration, data, "Transformation generated",0, 0, 0, PreprocessingInfo::before);
	firstDecl->addToAttachedPreprocessingInfo(result, PreprocessingInfo::after);
}
void BasicProgmemTransform::convertVarDeclToProgmemDecl(SgVariableDeclaration *varDecl) {
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	std::string dec = "const char ";
	SgInitializedName *initName = varDecl->get_variables()[0];
	std::string literal = isSgAssignInitializer(initName->get_initializer())->get_operand()->unparseToString();
	dec += initName->get_name().getString() + "[] PROGMEM =" + literal + ";";
	insertPreprocessingInfo(dec);
	SageInterface::removeStatement(varDecl, true);
}

void BasicProgmemTransform::setupCharBufferForFunction(SgFunctionDeclaration *func) {
	int sizeNeeded = getBuffersizeNeededForFunction(func);
	if(sizeNeeded == 0) {
		return;
	}
	SgArrayType* arrType = SageBuilder::buildArrayType(SageBuilder::buildCharType(), SageBuilder::buildIntVal(sizeNeeded));
	SgScopeStatement *scope = func->get_definition()->get_body();
	SgVariableDeclaration *decl = SageBuilder::buildVariableDeclaration(FUNC_BUFFER_NAME, arrType, NULL, scope);
	SageInterface::prependStatement(decl, scope);
}

void BasicProgmemTransform::castProgmemParams(SgFunctionCallExp* funcCall, SgVarRefExp *var) {
	SageInterface::addTextForUnparser(var, "FS(", AstUnparseAttribute::e_before);
	SageInterface::addTextForUnparser(var, ")", AstUnparseAttribute::e_after);
}

void BasicProgmemTransform::loadProgmemStringsIntoBuffer(SgFunctionCallExp *funcCall, SgVarRefExp *var, int& pos) {
	//strcpy_P(&buff[pos], ...);
	int length = getSizeNeededToLoadFromProgmem(var);
	if(length == 0) { return; }
	std::stringstream instr;
	instr << "\n strcpy_P(&" << FUNC_BUFFER_NAME << "[";
	instr << pos << "], " << var->get_symbol()->get_name().getString() << ");\n";
	SgNode *stmt = funcCall;
	while(isSgExprStatement(stmt) == NULL && stmt->get_parent() != NULL){
		stmt = stmt->get_parent();
	}
	SageInterface::addTextForUnparser(stmt, instr.str(), AstUnparseAttribute::e_before);

	std::stringstream arr;
	arr << "&" << FUNC_BUFFER_NAME <<"[" <<pos << "]/*";
//	SgVarRefExp *buff = SageBuilder::buildVarRefExp(FUNC_BUFFER_NAME);
//	SgPntrArrRefExp *buffExp = new SgPntrArrRefExp(buff, SageBuilder::buildIntVal(pos), SageBuilder::buildArrayType(SageBuilder::buildCharType()));
//	SgAddressOfOp *buffAddr = SageBuilder::buildAddressOfOp(buffExp);
//
//	SageInterface::replaceExpression(var, buffAddr);
	SageInterface::addTextForUnparser(var, arr.str(), AstUnparseAttribute::e_before);
	SageInterface::addTextForUnparser(var, "*/", AstUnparseAttribute::e_after);
	pos += length;

}

void BasicProgmemTransform::transformFunction(SgFunctionDeclaration *func) {
	setupCharBufferForFunction(func);
	Rose_STL_Container<SgNode *> funcCalls = NodeQuery::querySubTree(func, V_SgFunctionCallExp);
	int startPos = 0;
	for(auto &funcCall: funcCalls) {
		SgFunctionCallExp *fcall = isSgFunctionCallExp(funcCall);
		Function callee(fcall);
		bool arduinoP = isArduinoProgmemSafeFunction(callee);
		SgExpressionPtrList params = fcall->get_args()->get_expressions();
		//TODO: figure out how to wrap with macro
		for(auto &expr: params) {
			SgVarRefExp* var = isSgVarRefExp(expr);
			if(var == NULL) { continue ;}
			SgInitializedName *initName = var->get_symbol()->get_declaration();
			if(isVarDeclToRemove(initName)) {
				if(arduinoP) {
					castProgmemParams(fcall, var);
				} else {
					loadProgmemStringsIntoBuffer(fcall, var, startPos);
				}
			}
		}
	}
}

std::set<varID> BasicProgmemTransform::getVarsBoundToNonPlaceholderPointers() {
	std::set<varID> results;
	Rose_STL_Container<SgNode *> varRefs = NodeQuery::querySubTree(project, V_SgVarRefExp);
	for(auto &ref: varRefs) {
		SgVarRefExp *var = isSgVarRefExp(ref);
		if(isFromLibrary(var->get_symbol()->get_declaration())){
			continue;
		}
		varID varRef = SgExpr2Var(var);
		if(aliasAnalysis->variableAtNodeHasKnownAlias(var, varRef) == false || var->isUsedAsLValue()) {
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
//	printf("getting globals...\n");
	for(auto& global:globals) {
//		printf("checking.. %s\n", global->unparseToString().c_str());
		SgExpression *assigned = getRHSOfVarDecl(global);
		if(assigned == NULL) { continue; }
		if(isSgStringVal(assigned)) {
			varID placeholder = sla->getPlaceholderVarIDForStringLiteral(isSgStringVal(assigned)->get_value());
			if(safePlaceholders.find(placeholder) != safePlaceholders.end()) {
				SgVariableDeclaration *decl = isSgVariableDeclaration(global->get_declaration());
				if(decl) {
//					printf("shifting %s\n", decl->unparseToString().c_str());
					varDeclsToShift.insert(decl);
				}
			}
		}
	}
	return;
}

SgExpression* BasicProgmemTransform::getRHSOfVarDecl(SgInitializedName *initName) {
	SgAssignInitializer* init = isSgAssignInitializer(initName->get_initializer());
	if(init == NULL) {
		return NULL;
	}
	return init->get_operand();
}

bool BasicProgmemTransform::isVarDeclToRemove(SgInitializedName *initName) {
	SgVariableDeclaration *varDec = isSgVariableDeclaration(initName->get_declaration());
	if(varDec == NULL) {
		return false;
	}
	if(varDeclsToShift.find(varDec) != varDeclsToShift.end()) {
		return true;
	}
	return false;
}
