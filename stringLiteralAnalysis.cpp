#include "stringLiteralAnalysis.h"
#include "functionState.h"
#include <algorithm>
#include <functional>

bool StringLiteralInfo::addFuncOccurance(SgFunctionDeclaration * func, SgStatement *stmt) {
	if(funcOccurances.find(func) == funcOccurances.end()){
		StatementList *lst = new StatementList();
		lst->push_back(stmt);
		funcOccurances[func] = lst;
		return true;
	} else {
		funcOccurances[func]->push_back(stmt);
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
	for(auto const& item:  funcOccurances){
		out << item.first->get_name().getString();
		out << ";;";
	}
	out << "] \n";
	return out.str();
}

//Implementation of analysis

void StringLiteralAnalysis::runAnalysis() {
	StringLiteralAnalysisVisitor visitor(this);
	visitor.traverseInputFiles(project);
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


StringLiteralAnalysisVisitor::StringLiteralAnalysisVisitor(StringLiteralAnalysis *analysis){
	this->analyser = analysis;
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
	int numOcc = 0;
	if(!declStack.empty()){
		SgFunctionDeclaration *decl = declStack.top();
		printf(" function name %s\n", decl->get_name().getString().c_str());
		sInfo.addFuncOccurance(decl, p);
		numOcc = sInfo.getFuncOccuranceNum();
	}

	if(numOcc >1 || numOcc == 0) {
		analyser->globalStrLiterals.insert(item);
	}
}

void StringLiteralAnalysisVisitor::preOrderVisit(SgNode *node){
	if(isSgFunctionDeclaration(node)){
			declStack.push(isSgFunctionDeclaration(node));
			printf("function name %s\n", isSgFunctionDeclaration(node)->get_name().getString().c_str());
	}
	if(isSgStatement(node)) {
		stmtStack.push(isSgStatement(node));
	}else if(isSgStringVal(node)){
		visitStringVal(isSgStringVal(node));
	}
}

void StringLiteralAnalysisVisitor::postOrderVisit(SgNode *node){
	if(isSgFunctionDeclaration(node)){
			declStack.pop();
			printf("popped function name %s\n", isSgFunctionDeclaration(node)->get_name().getString().c_str());
	}
	if(isSgStatement(node)) {
		stmtStack.pop();
	}
}

//StringLiteralAnalysisVisitor::~StringLiteralAnalysisVisitor(){
//	printf("destroyed visitor\n");
//}
