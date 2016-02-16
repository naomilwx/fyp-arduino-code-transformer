#include "stringLiteralAnalysis.h"

bool StringLiteralInfo::addFuncOccurance(SgFunctionDeclaration * func) {
	if(funcOccurances.find(func) == funcOccurances.end()){
		funcOccurances.insert(func);
		return true;
	}
	return false;
}

std::string StringLiteralInfo::getTag() const {
	return tag;
}

int StringLiteralInfo::getFuncOccuranceNum() {
	return funcOccurances.size();
}

FunctionInfo StringLiteralAnalysis::evaluateInheritedAttribute(SgNode *node, FunctionInfo info) {
	SgFunctionDeclaration *dec = dynamic_cast<SgFunctionDeclaration *>(node);
	if(dec != NULL) {
		return FunctionInfo(dec);
	}

	SgStringVal* literal = dynamic_cast<SgStringVal*>(node);
	if(literal != NULL) {
		std::string item = literal->get_value();
		if(strLiterals.find(item) == strLiterals.end()){
			//First time string literal is found
			strCount++;
			StringLiteralInfo *t = new StringLiteralInfo(strCount);
			strLiterals[item] = *t;
		}
		StringLiteralInfo sInfo = strLiterals[item];
		sInfo.addFuncOccurance(info.declaration);
		if(sInfo.getFuncOccuranceNum() > 1) {
			globalStrLiterals.insert(item);
		}

	}
	return info;

}

std::string StringLiteralAnalysis::getStringLiteralLabel(std::string literal){
	if(strLiterals.find(literal) != strLiterals.end()) {
		return strLiterals[literal].getTag();
	}
	return "";
}

int StringLiteralAnalysis::getNumberOfStringLiterals(){
	return strCount;
}

//void StringLiteralAnalysis::visit(SgNode* n){
//	SgStringVal* literal = dynamic_cast<SgStringVal*>(n);
//	if(literal != NULL){
//		std::string item = literal.get_value();
//		if(strLiterals.find(item) == strLiterals.end()){
//			strCount++;
//			strLiterals[item] = STRING_LITERAL_PREFIX + std::to_string(strCount);
//		}
//	}
//}
