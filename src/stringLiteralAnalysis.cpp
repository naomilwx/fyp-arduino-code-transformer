#include "stringLiteralAnalysis.h"
#include "functionState.h"
#include <algorithm>
#include <functional>


bool isStringLiteralPlaceholder(const std::string& str) {
	return str.substr(0, STRING_LITERAL_PREFIX.length()) == STRING_LITERAL_PREFIX;
}

bool StringLiteralInfo::addFuncOccurance(SgFunctionDeclaration * func, SgStatement *stmt) {
	bool changed = false;
	if(funcOccurances.find(func) == funcOccurances.end()){
		changed = true;
	}
	funcOccurances[func].push_back(stmt);
	return changed;
}

std::string StringLiteralInfo::getTag() const {
	return tag;
}

int StringLiteralInfo::getNumOccurances() const {
	return numOccurances;
}

int StringLiteralInfo::getFuncOccuranceNum() const {
	return funcOccurances.size();
}


varID StringLiteralInfo::getVarIDForPlaceholder() const {
	SgInitializedName *name = placeholder->get_variables().at(0);
	return varID(name);
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

void StringLiteralInfo::incNumOccurance() {
	numOccurances+=1;
}
bool StringLiteralInfo::occursInFunc(SgFunctionDeclaration *func) const{
	return funcOccurances.find(func) != funcOccurances.end();
}
//Implementation of analysis

void StringLiteralAnalysis::runAnalysis() {
	StringLiteralAnalysisVisitor visitor(this);
	visitor.traverseInputFiles(project);
}

long StringLiteralAnalysis::getTotalStringSize() {
	long total = 0;
	for(auto const& item: strLiterals) {
		total += item.first.size() * item.second.getNumOccurances();
	}
	return total;
}

std::string StringLiteralAnalysis::getStringLiteralLabel(const std::string& literal){
	if(strLiterals.find(literal) != strLiterals.end()) {
		return strLiterals[literal].getTag();
	}
	return "";
}

SgVariableDeclaration * StringLiteralAnalysis::getPlaceholderForStringLiteral(const std::string& literal){
	return strLiterals[literal].placeholder;
}

varID StringLiteralAnalysis::getPlaceholderVarIDForStringLiteral(const std::string& literal) {
	return strLiterals[literal].getVarIDForPlaceholder();
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

std::set<varID> StringLiteralAnalysis::getPlaceholderVarIDs() {
	std::set<varID> result;
	for(auto const& item: strLiterals) {
		result.insert(item.second.getVarIDForPlaceholder());
	}
	return result;
}

std::string StringLiteralAnalysis::getStringLiteralForLabel(const std::string& label) {
	for(auto const& item: strLiterals) {
                if(item.second.getTag() == label) {
			return item.first;
		}
        }
	return "";
}
StringLiteralInfo StringLiteralAnalysis::getStringLiteralInfo(const std::string&  literal) {
	if(strLiterals.find(literal) != strLiterals.end()) {
		return strLiterals[literal];
	}
	return StringLiteralInfo();
}

//StatementLiteralMap* StringLiteralAnalysis::getStatementLiteralMap() {
//	return &slMap;
//}

LiteralMap* StringLiteralAnalysis::getLiteralMap() {
	return &strLiterals;
}

StringSet StringLiteralAnalysis::getStringLiteralsInFunction(SgFunctionDeclaration *func) {
	StringSet result;
	for(auto const& item: strLiterals) {
		if(item.second.occursInFunc(func)){
			result.insert(item.first);
		}
	}
	return result;
}

std::string StringLiteralAnalysis::getAnalysisPrintout(){
	std::ostringstream out;
	/*for(auto const& item: strLiterals) {
		out << item.first << ": \n";
		out << "  " << "details:";
		out << item.second.getSummaryPrintout();
	}*/
	out << "Total string length: " << getTotalStringSize();
	return out.str();
}


StringLiteralAnalysisVisitor::StringLiteralAnalysisVisitor(StringLiteralAnalysis *analysis): declStack(), stmtStack(){
	this->analyser = analysis;
}

void StringLiteralAnalysisVisitor::visitStringVal(SgStringVal *node){
	SgStatement *p = stmtStack.top();
	//	printf("wrapping stmt: %s\n", p->class_name().c_str());
	SgGlobal *global = SageInterface::getFirstGlobalScope(analyser->project);
	const std::string& item = node->get_value();
	if(analyser->strLiterals.find(item) == analyser->strLiterals.end()){
		//First time string literal is found
		analyser->strCount++;
		StringLiteralInfo t(analyser->strCount, global);
		analyser->strLiterals[item] = t;
	}
	StringLiteralInfo &sInfo = analyser->strLiterals[item];
	sInfo.incNumOccurance();
	if(!declStack.empty()){
		SgFunctionDeclaration *decl = declStack.top();
		//		printf(" function name %s\n", decl->get_name().getString().c_str());
		sInfo.addFuncOccurance(decl, p);
	}

//	analyser->slMap[p].insert(item);
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

