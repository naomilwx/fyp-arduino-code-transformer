#ifndef _STRINGLITERALANALYSIS_H
#define _STRINGLITERALANALYSIS_H
#include "rose.h"
#include <string>
#include <sstream>



template < typename T > std::string to_string( const T& n ) {
    std::ostringstream stm ;
    stm << n ;
    return stm.str() ;
 }

typedef std::set<std::string> StringSet;

const std::string STRING_LITERAL_PREFIX = "__STRLIT_";

class StringLiteralInfo {
	typedef  std::vector<SgStatement *> StatementList;
	typedef std::map<SgFunctionDeclaration *, StatementList> FunctionMap;

	std::string tag;
	FunctionMap funcOccurances; //Map of function decl to the statements containing the string literal
	SgVariableDeclaration *placeholder;

	public:
	StringLiteralInfo(): funcOccurances(), tag() {
		placeholder = NULL;
		}

	StringLiteralInfo(int tagNum, SgGlobal *global) {
		tag = STRING_LITERAL_PREFIX + to_string(tagNum);
		SgType *type = SageBuilder::buildPointerType(SageBuilder::buildConstType(SageBuilder::buildCharType()));
		placeholder = SageBuilder::buildVariableDeclaration(tag, type, NULL, global);
	}

	std::string getTag() const;

	int getFuncOccuranceNum() const;
	std::string getSummaryPrintout() const;
	SgVariableDeclaration *getPlaceholder() {
		return placeholder;
	}
	bool occursInFunc(SgFunctionDeclaration *func) const;
	protected:
	bool addFuncOccurance(SgFunctionDeclaration * func, SgStatement* stmt);

	friend class StringLiteralAnalysis;
	friend class StringLiteralAnalysisVisitor;
};

typedef std::map<std::string, StringLiteralInfo> LiteralMap;
typedef std::map<SgStatement *, StringSet> StatementLiteralMap;

class StringLiteralAnalysis {
protected:
	int strCount;
	StringSet globalStrLiterals; //String literals used in more than one function
	LiteralMap strLiterals; //Map of string literals to the statements where they are used, the functions where they occur and the tag assigned to them
	StatementLiteralMap slMap; //Map of statements to the string literals used in the statements
	SgProject *project;


public:
	StringLiteralAnalysis(SgProject *project): strCount(0), globalStrLiterals(), strLiterals(), slMap(){
		this->project = project;
	}
	void runAnalysis();

	std::string getStringLiteralLabel(const std::string& literal);
	std::string getStringLiteralForLabel(const std::string& label);
	StringSet getStringLiteralsInFunction(SgFunctionDeclaration *func);
	SgVariableDeclaration *getPlaceholderForStringLiteral(const std::string& literal);
	StringSet getStringLiterals();
	int getNumberOfStringLiterals();
	bool isGlobalStringLiteral(const std::string& str);
	StringLiteralInfo getStringLiteralInfo(const std::string&  literal);
	StatementLiteralMap* getStatementLiteralMap();
	LiteralMap *getLiteralMap();
	std::string getAnalysisPrintout();

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

void addProgmemStringLiterals(SgProject *project, LiteralMap *lMap);
#endif
