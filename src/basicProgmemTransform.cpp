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
		transformStringConstructors(func);
		transformArrayRef(func);
	}
	transformCharArrayInitialization();
	printf("ok\n");
	insertProgmentCastHeader();
	shiftVarDeclsToProgmem();
}

void BasicProgmemTransform::transformArrayRef(SgFunctionDeclaration *func) {
	Rose_STL_Container<SgNode *> refs = NodeQuery::querySubTree(func, V_SgPntrArrRefExp);
	for(auto& item: refs) {
		SgPntrArrRefExp *exp = isSgPntrArrRefExp(item);
		SgVarRefExp *var = isSgVarRefExp(exp->get_lhs_operand());
		if(var == NULL) {
			continue;
		}
		if(isVarDeclToRemove(var)) {
			handleProgmemArrayIndexRef(exp);
		}
	}
}

void BasicProgmemTransform::transformCharArrayInitialization() {
	/* *
	 * Translates statements of the form:
	 * char arr[n] = "some string"; to:
	 * char arr[n];
	 * strcpy_P(arr, <progmem placeholder>);
	 * */
	Rose_STL_Container<SgNode *> initNames = NodeQuery::querySubTree(project, V_SgInitializedName);
	for(auto &item: initNames) {
		SgInitializedName *initName = isSgInitializedName(item);
		if(initName->get_initializer() == NULL) {
			continue;
		}
		SgVariableDeclaration * varDecl = isSgVariableDeclaration(initName->get_declaration());
		if(varDecl == NULL) {
			continue;
		}
		SgAssignInitializer *assignInit = isSgAssignInitializer(initName->get_initializer());
		if(assignInit == NULL) {
			continue;
		}
		SgType *type = initName->get_type();
		SgType *eleType = SageInterface::getElementType(type);
		SgScopeStatement *scope = SageInterface::getGlobalScope(initName);
		if(isSgArrayType(type) && eleType != NULL && isSgTypeChar(eleType)) {
			SgStringVal* strVal = isSgStringVal(assignInit->get_operand());
			std::string str = strVal->get_value();
			int arrSize = getDeclaredArraySize(isSgArrayType(type));
			if(arrSize == 0) {
				//char arr[] = "something";
				int size = str.length() + 1;
				SgArrayType *type = SageBuilder::buildArrayType(SageBuilder::buildCharType(), SageBuilder::buildIntVal(size));
				initName->set_type(type);
			}
			varDecl->reset_initializer(NULL);
			SgVariableDeclaration *placeholder = getVariableDeclPlaceholderForString(str, scope);
			SgVarRefExp *ref = SageBuilder::buildVarRefExp(placeholder);
			std::stringstream instr;
			instr << "\n strcpy_P(" << initName->get_name().getString();
			instr <<  ", " << ref->get_symbol()->get_name().getString() << ");\n";
			SageInterface::attachComment(varDecl, instr.str(), PreprocessingInfo::after);
			printf("transformed %s\n", initName->unparseToString().c_str());
		}

	}
}

SgVariableDeclaration *BasicProgmemTransform::getVariableDeclPlaceholderForString(const std::string& str, SgScopeStatement *scope) {
	for(auto &varDecl: varDeclsToShift) {
		SgInitializedName *initName = varDecl->get_variables().at(0);
		SgAssignInitializer *assign = isSgAssignInitializer(initName->get_initializer());
		if(assign == NULL) {
			continue; //Should not happen
		}
		SgStringVal* strVal = isSgStringVal(assign->get_operand());
		if(strVal->get_value() == str) {
			return varDecl;
		}
	}

	if(additionalProgmemStrings[scope].find(str) != additionalProgmemStrings[scope].end()) {
		return (additionalProgmemStrings[scope])[str];
	}

	std::string placeholder = sla->getStringLiteralLabel(str);
	SgType *type = SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
	SgAssignInitializer *initializer = SageBuilder::buildAssignInitializer(SageBuilder::buildStringVal(str));
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);

	SgVariableDeclaration *varDec = SageBuilder::buildVariableDeclaration( "ar" +placeholder, type, initializer, global);
	(additionalProgmemStrings[scope])[str] = varDec;
	return varDec;
}

int BasicProgmemTransform::getDeclaredArraySize(SgArrayType *arrType) {
	SgExpression *arrIndex = arrType->get_index();
	if(arrIndex == NULL) {
		return 0;
	}
	if(isSgIntVal(arrIndex)) {
		return isSgIntVal(arrIndex)->get_value();
	}
	return 0;
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

void BasicProgmemTransform::handleProgmemArrayIndexRef(SgPntrArrRefExp *ref) {
	SgType *type = ref->get_type();
	if(isSgTypeChar(type)) {
		//character array
		SageInterface::addTextForUnparser(ref, "(char)pgm_read_byte(&(", AstUnparseAttribute::e_before);
		SageInterface::addTextForUnparser(ref, "))", AstUnparseAttribute::e_after);
	} else if(SageInterface::isPointerType(type) && isSgTypeChar(type->findBaseType())) {
		SageInterface::addTextForUnparser(ref, "(char *)pgm_read_word(&(", AstUnparseAttribute::e_before);
		SageInterface::addTextForUnparser(ref, "))", AstUnparseAttribute::e_after);
	}
}

int BasicProgmemTransform::getSizeNeededToLoadFromProgmem(SgVarRefExp *var) {
	SgInitializedName *initName = var->get_symbol()->get_declaration();
	SgExpression *rhs = getRHSOfVarDecl(initName);
	if(rhs && isSgStringVal(rhs)) {
		return isSgStringVal(rhs)->get_value().size() + 1;
	}
	return 0;
}

void BasicProgmemTransform::insertProgmentCastHeader() {
	SgFilePtrList files = project->get_files();
	for(SgFile *file: files){
		SgSourceFile *sourceFile = isSgSourceFile(file);
		if(sourceFile && sourceFile->get_globalScope()){
			std::string flashHelper = "#define FS(x)(__FlashStringHelper*)(x)";
			insertPreprocessingInfo(flashHelper, sourceFile->get_globalScope());
		}
	}

}

void BasicProgmemTransform::shiftVarDeclsToProgmem() {

	printf("shifting var decls...\n");
	for(auto& varDecl : varDeclsToShift) {
		convertVarDeclToProgmemDecl(varDecl);
	}
	printf("shifting additional progmem strings...\n");
	for(auto &outerMap: additionalProgmemStrings) {
		for(auto &item: outerMap.second) {
			SgInitializedName *initName = item.second->get_variables()[0];
			std::string dec = "const char " + initName->get_name().getString() + "[] PROGMEM = \"" + item.first + "\";";
			insertPreprocessingInfo(dec, outerMap.first);
		}
	}

	printf("ok here\n");
}
void BasicProgmemTransform::insertPreprocessingInfo(const std::string &data, SgScopeStatement *scope) {
//	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	SgStatement *firstDecl = SageInterface::getFirstStatement(scope);
	PreprocessingInfo* result = new PreprocessingInfo(PreprocessingInfo::CpreprocessorIncludeDeclaration, data, "Transformation generated",0, 0, 0, PreprocessingInfo::before);
	firstDecl->addToAttachedPreprocessingInfo(result, PreprocessingInfo::after);
}
void BasicProgmemTransform::convertVarDeclToProgmemDecl(SgVariableDeclaration *varDecl) {
	printf("converting %s\n", varDecl->unparseToString().c_str());
	std::string dec = "const char ";
	SgInitializedName *initName = varDecl->get_variables()[0];
	std::string literal = isSgAssignInitializer(initName->get_initializer())->get_operand()->unparseToString();
	dec += initName->get_name().getString() + "[] PROGMEM =" + literal + ";";
	SgScopeStatement *scope = SageInterface::getGlobalScope(initName);
	insertPreprocessingInfo(dec, scope);
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

void BasicProgmemTransform::castProgmemParams(SgVarRefExp *var) {
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
	SageInterface::addTextForUnparser(var, arr.str(), AstUnparseAttribute::e_before);
	SageInterface::addTextForUnparser(var, "*/", AstUnparseAttribute::e_after);
	pos += length;

}

void BasicProgmemTransform::loadReplacewithProgmemFunction(SgFunctionCallExp *funcCall, std::string replacement) {
	SgName newName(replacement);
	SgFunctionCallExp *newFuncCall = SageBuilder::buildFunctionCallExp(newName, funcCall->get_type(), funcCall->get_args(), SageInterface::getGlobalScope(funcCall));
	SageInterface::replaceExpression(funcCall, newFuncCall, true);
}


void BasicProgmemTransform::transformStringConstructors(SgFunctionDeclaration *func) {
	Rose_STL_Container<SgNode *> constructors = NodeQuery::querySubTree(func, V_SgConstructorInitializer);
	for(auto &cons: constructors) {
		SgConstructorInitializer *consInit = isSgConstructorInitializer(cons);
		SgClassDeclaration *classDecl = consInit->get_class_decl();
		if(isArduinoStringType(classDecl->get_type())) {
			SgExpressionPtrList exprs = consInit->get_args()->get_expressions();
			for(auto &exp: exprs) {
				SgVarRefExp* var = isSgVarRefExp(exp);
				if(var == NULL) { continue ;}
//				SgInitializedName *initName = var->get_symbol()->get_declaration();
				if(isVarDeclToRemove(var)) {
					castProgmemParams(var);
				}
			}
		}

	}
}

void BasicProgmemTransform::transformFunction(SgFunctionDeclaration *func) {
	setupCharBufferForFunction(func);
	Rose_STL_Container<SgNode *> funcCalls = NodeQuery::querySubTree(func, V_SgFunctionCallExp);

	for(auto &funcCall: funcCalls) {
		int startPos = 0;
		SgFunctionCallExp *fcall = isSgFunctionCallExp(funcCall);
		Function callee(fcall);
		std::string orig = callee.get_name().getString();
		std::string replacement = getReplacementName(orig);
		bool arduinoP = isArduinoProgmemSafeFunction(callee);
		SgExpressionPtrList params = fcall->get_args()->get_expressions();
		param_pos_list progmemPositions = getPositionsToIgnore(orig);
		//TODO: figure out how to wrap with macro
		int index = 0;
		for(auto &expr: params) {
			SgVarRefExp* var = isSgVarRefExp(expr);
			if(var == NULL) { continue;}
			if(isVarDeclToRemove(var)) {
				if(arduinoP) {
					castProgmemParams(var);
				} else if(progmemPositions.find(index) == progmemPositions.end()) {
					loadProgmemStringsIntoBuffer(fcall, var, startPos);
				}
			}
			index++;
		}
		if(replacement != "") {
			loadReplacewithProgmemFunction(fcall, replacement);
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
		if(varRef.str().find(STRING_LITERAL_PREFIX) == std::string::npos) {
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

std::set<varID> BasicProgmemTransform::getVarsInUnsafeConstructors() {
	std::set<varID> results;
	Rose_STL_Container<SgNode *> constructors = NodeQuery::querySubTree(project, V_SgConstructorInitializer);
	for(auto& cons: constructors) {
		SgConstructorInitializer *consInit = isSgConstructorInitializer(cons);
		SgClassDeclaration *classDecl = consInit->get_class_decl();
		if(isArduinoStringType(classDecl->get_type()) == false) {
			SgExpressionPtrList exprs = consInit->get_args()->get_expressions();
			for(auto &exp: exprs) {
				SgVarRefExp* var = isSgVarRefExp(exp);
				if(var == NULL) { continue ;}
				std::set<varID> aliases = aliasAnalysis->getAliasesForVariableAtNode(var, SgExpr2Var(var));
				results.insert(aliases.begin(), aliases.end());
			}
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

std::set<varID> BasicProgmemTransform::getAllUnsafeVars() {
	std::set<varID> results;
	std::set<varID> varsInFuncRet = getVarsReturnedByFunctions();
	std::set<varID> varsInUnsafe = getVarsInUnsafeFunctionCalls();
	std::set<varID> varsBound = getVarsBoundToNonPlaceholderPointers();
	std::set<varID> inUnsafeCons = getVarsInUnsafeConstructors();
	results.insert(varsInFuncRet.begin(), varsInFuncRet.end());
	results.insert(varsInUnsafe.begin(), varsInUnsafe.end());
	results.insert(varsBound.begin(), varsBound.end());
	results.insert(inUnsafeCons.begin(), inUnsafeCons.end());
	return results;
}

void BasicProgmemTransform::setupProgmemableVarDecls() {
	std::vector<SgInitializedName *> globals = getGlobalVars(project);
	std::set<varID> unsafeVars = getAllUnsafeVars();
//	printf("getting globals...\n");
	for(SgInitializedName * global:globals) {
//		printf("checking.. %s\n", global->unparseToString().c_str());
		if(isCharArrayType(global->get_type())) {
			continue;
		}
		SgExpression *assigned = getRHSOfVarDecl(global);
		if(assigned == NULL) { continue; }
		if(isSgStringVal(assigned)) {
			varID placeholder = sla->getPlaceholderVarIDForStringLiteral(isSgStringVal(assigned)->get_value());
			if(unsafeVars.find(placeholder) == unsafeVars.end()) {
				SgVariableDeclaration *decl = isSgVariableDeclaration(global->get_declaration());
				if(decl) {
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

bool BasicProgmemTransform::isVarDeclToRemove(SgVarRefExp *var) {
	SgInitializedName *initName = var->get_symbol()->get_declaration();
	return isVarDeclToRemove(initName);
}
