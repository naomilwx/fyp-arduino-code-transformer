#include "stringLiteralAnalysis.h"
#include "functionState.h"
#include <algorithm>
#include <functional>

bool StringLiteralInfo::addFuncOccurance(SgFunctionDeclaration * func, SgStatement *stmt) {
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

void StringLiteralAnalysis::runAnalysis() {
	set<FunctionState*>& funcStates = FunctionState::getAllDefinedFuncs();
	for(FunctionState *fs : funcStates){
		SgFunctionDeclaration* decl = fs->func.get_declaration();
		if(decl->get_definition()){
			printf("function name %s\n", decl->get_name().getString().c_str());
			StringLiteralAnalysisVisitor visitor(this, decl);
			visitor.traverse(decl->get_definition());
//			SgBasicBlock *funcBlock = decl->get_definition()->get_body();
//			for(SgStatement *stmt: funcBlock->get_statements()){
//				StringLiteralAnalysisVisitor visitor(this, decl, stmt);
//				visitor.traverse(stmt, preorder);
//			}
		}

	}
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


StringLiteralAnalysisVisitor::StringLiteralAnalysisVisitor(StringLiteralAnalysis *analysis, SgFunctionDeclaration* func){
	this->analyser = analysis;
	this->decl = func;
}

void StringLiteralAnalysisVisitor::visitStringVal(SgStringVal *node){
	SgStatement *p = stmtStack.top();
	printf("wrapping stmt: %s\n", p->class_name().c_str());
	std::string item = node->get_value();
	if(analyser->strLiterals.find(item) == analyser->strLiterals.end()){
				//First time string literal is found
		analyser->strCount++;
		StringLiteralInfo *t = new StringLiteralInfo(analyser->strCount);
		analyser->strLiterals[item] = *t;
	}
	StringLiteralInfo &sInfo = analyser->strLiterals[item];
	sInfo.addFuncOccurance(decl, p);
	int numOcc = sInfo.getFuncOccuranceNum();
	if(numOcc >1 || numOcc == 0) {
		analyser->globalStrLiterals.insert(item);
	}
}

void StringLiteralAnalysisVisitor::preOrderVisit(SgNode *node){
	if(isSgStatement(node)) {
		stmtStack.push(isSgStatement(node));
	}else if(isSgStringVal(node)){
		visitStringVal(isSgStringVal(node));
	}
	printf("start of visit: %s\n", node->class_name().c_str());
}

void StringLiteralAnalysisVisitor::postOrderVisit(SgNode *node){
	if(isSgStatement(node)) {
			stmtStack.pop();
	}
	printf("end of visit: %s\n", node->class_name().c_str());
}

//StringLiteralAnalysisVisitor::~StringLiteralAnalysisVisitor(){
//	printf("destroyed visitor\n");
//}
