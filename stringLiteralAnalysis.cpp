#include "stringLiteralAnalysis.h"

bool StringLiteralInfo::addFuncOccurance(SgFunctionDeclaration * func) {
	if(func != NULL && funcOccurances.find(func) == funcOccurances.end()){
		funcOccurances.insert(func);
		return true;
	}
	return false;
}

std::string StringLiteralInfo::getTag() const {
	return tag;
}

int StringLiteralInfo::getFuncOccuranceNum() const {
	return funcOccurances.size();
}

std::string StringLiteralInfo::getSummaryPrintout() const{
	std::ostringstream out;
	out << "tag: " << getTag() << "\n";
	out<< "num of functions :"<< to_string(this->getFuncOccuranceNum()) << "\n";
	out << "func occurs=[";
	for(FunctionSet::iterator it = funcOccurances.begin(); it!=funcOccurances.end(); it++){
		out << (*it)->get_name().getString();
		out << ";;";
	}
	out << "] \n";
	return out.str();
}

//Implementation of analysis

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
		StringLiteralInfo &sInfo = strLiterals[item];
		sInfo.addFuncOccurance(info.declaration);
		int numOcc = sInfo.getFuncOccuranceNum();
		if(numOcc >1 || numOcc == 0) {
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

std::set<std::string> StringLiteralAnalysis::getStringLiterals() {
	std::set<std::string> strSet;
	for(auto const& item: strLiterals) {
		strSet.insert(item.first);
	}
	return strSet;
}

StringLiteralInfo StringLiteralAnalysis::getStringLiteralInfo(std::string literal) {
	if(strLiterals.find(literal) != strLiterals.end()) {
		return strLiterals[literal];
	}
	return StringLiteralInfo();
}

bool StringLiteralAnalysis::isGlobalStringLiteral(std::string str) {
	return globalStrLiterals.find(str) != globalStrLiterals.end();
}

std::string StringLiteralAnalysis::getAnalysisPrintout(){
	std::ostringstream out;
	for(auto const& item: strLiterals) {
		out << item.first << ": \n";
		out << "  " << "details:";
		out << item.second.getSummaryPrintout();
	}
	out << "global string literals: [";
	for(auto it = globalStrLiterals.begin(); it!=globalStrLiterals.end(); it++){
		out << (*it);
		out << ";;";
	}
	out << "] \n";
	return out.str();
}
