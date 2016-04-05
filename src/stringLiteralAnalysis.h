#ifndef _STRINGLITERALANALYSIS_H
#define _STRINGLITERALANALYSIS_H
#include "rose.h"
#include <string>
#include <sstream>

#include "variables.h"

typedef std::set<std::string> StringSet;

template < typename T > std::string to_string( const T& n ) {
    std::ostringstream stm ;
    stm << n ;
    return stm.str() ;
 }


const std::string STRING_LITERAL_PREFIX = "_STRLT_";
bool isStringLiteralPlaceholder(const std::string& str);

class StringLiteralInfo {
	typedef  std::vector<SgStatement *> StatementList;
	typedef std::map<SgFunctionDeclaration *, StatementList> FunctionMap;

	std::string tag;
	FunctionMap funcOccurances; //Map of function decl to the statements containing the string literal
	SgVariableDeclaration *placeholder;
	int numOccurances;

	public:
	StringLiteralInfo(): funcOccurances(), tag(), numOccurances(0){
		placeholder = NULL;
		}

	StringLiteralInfo(int tagNum, SgGlobal *global): numOccurances(0) {
		tag = STRING_LITERAL_PREFIX + to_string(tagNum);
		SgType *type = SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
		placeholder = SageBuilder::buildVariableDeclaration_nfi(tag, type, NULL, global);
	}

	std::string getTag() const;
	int getNumOccurances() const;
	int getFuncOccuranceNum() const;
	std::string getSummaryPrintout() const;

	void incNumOccurance();
	SgVariableDeclaration *getPlaceholder() {
		return placeholder;
	};

	varID getVarIDForPlaceholder() const;

	bool occursInFunc(SgFunctionDeclaration *func) const;
	protected:
	bool addFuncOccurance(SgFunctionDeclaration * func, SgStatement* stmt);

	friend class StringLiteralAnalysis;
	friend class StringLiteralAnalysisVisitor;
};

typedef std::map<std::string, StringLiteralInfo> LiteralMap;

class StringLiteralAnalysis {
protected:
	int strCount;
	LiteralMap strLiterals; //Map of string literals to the statements where they are used, the functions where they occur and the tag assigned to them
	SgProject *project;


public:
	StringLiteralAnalysis(SgProject *project): strCount(0), strLiterals(){
		this->project = project;
	}
	void runAnalysis();

	std::string getStringLiteralLabel(const std::string& literal);
	std::string getStringLiteralForLabel(const std::string& label);
	StringSet getStringLiteralsInFunction(SgFunctionDeclaration *func);
	SgVariableDeclaration *getPlaceholderForStringLiteral(const std::string& literal);
	varID getPlaceholderVarIDForStringLiteral(const std::string& literal);
	StringSet getStringLiterals();
	std::set<varID> getPlaceholderVarIDs();
	int getNumberOfStringLiterals();
	StringLiteralInfo getStringLiteralInfo(const std::string&  literal);
	LiteralMap *getLiteralMap();
	std::string getAnalysisPrintout();
	long getTotalStringSize();
	friend class StringLiteralAnalysisVisitor;
};

class StringLiteralAnalysisVisitor: public AstPrePostProcessing {
protected:
	StringLiteralAnalysis *analyser;
	std::stack<SgFunctionDeclaration *> declStack;
	std::stack<SgStatement *> stmtStack;
public:
	StringLiteralAnalysisVisitor(StringLiteralAnalysis *a);
	void visitStringVal(SgStringVal *node);
	void preOrderVisit(SgNode *node);
	void postOrderVisit(SgNode *node);
};

#endif
