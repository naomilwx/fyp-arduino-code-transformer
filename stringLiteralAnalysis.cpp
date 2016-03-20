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

bool StringLiteralInfo::occursInFunc(SgFunctionDeclaration *func) const{
	return funcOccurances.find(func) != funcOccurances.end();
}
//Implementation of analysis

void StringLiteralAnalysis::runAnalysis() {
	StringLiteralAnalysisVisitor visitor(this);
	visitor.traverseInputFiles(project);
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

bool StringLiteralAnalysis::isGlobalStringLiteral(const std::string& str) {
	return globalStrLiterals.find(str) != globalStrLiterals.end();
}

StatementLiteralMap* StringLiteralAnalysis::getStatementLiteralMap() {
	return &slMap;
}

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
	int numFuncOcc = 0;
	if(!declStack.empty()){
		SgFunctionDeclaration *decl = declStack.top();
		//		printf(" function name %s\n", decl->get_name().getString().c_str());
		sInfo.addFuncOccurance(decl, p);
		numFuncOcc = sInfo.getFuncOccuranceNum();
	}

	if(numFuncOcc >1 || numFuncOcc == 0) {
		analyser->globalStrLiterals.insert(item);
	}

	analyser->slMap[p].insert(item);
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

void addProgmemStringLiterals(SgProject *project, LiteralMap *lMap){	
	SgGlobal *global = SageInterface::getFirstGlobalScope(project);
	std::string pre;
	for(auto &item: *lMap) {
		std::string literal = item.first;
		std::string tag = item.second.getTag();
		pre += "\n const char " + tag + "[] PROGMEM = \""+ literal +"\";";
	}
	SgDeclarationStatementPtrList & stmtList = global->get_declarations ();
	if (stmtList.size()>0) // the source file is not empty
	{
		for (SgDeclarationStatementPtrList::iterator j = stmtList.begin ();
				j != stmtList.end (); j++){
			//must have this judgement, otherwise wrong file will be modified!
			//It could also be the transformation generated statements with #include attached
			if ( ((*j)->get_file_info ())->isSameFile(global->get_file_info ())||
					((*j)->get_file_info ())->isTransformation()) {

				PreprocessingInfo* result = new PreprocessingInfo(PreprocessingInfo::CpreprocessorIncludeDeclaration, pre, "Transformation generated",0, 0, 0, PreprocessingInfo::before);
				(*j)->addToAttachedPreprocessingInfo(result, PreprocessingInfo::after);
				break;
			}

		}
	}else{
		PreprocessingInfo* result = new PreprocessingInfo(PreprocessingInfo::CpreprocessorIncludeDeclaration, pre, "Transformation generated",0, 0, 0, PreprocessingInfo::after);
		global->addToAttachedPreprocessingInfo(result, PreprocessingInfo::after);
	}

}
