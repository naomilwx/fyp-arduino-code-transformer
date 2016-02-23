#include "rose.h"
#include <string>
#include <sstream>

#define STRING_LITERAL_PREFIX "const_string_"

template < typename T > std::string to_string( const T& n ) {
    std::ostringstream stm ;
    stm << n ;
    return stm.str() ;
 }

class StringLiteralInfo {
//	typedef std::set<SgFunctionDeclaration *> FunctionSet;
	typedef  std::vector<SgStatement *> StatementList;
	typedef std::map<SgFunctionDeclaration *, StatementList *> FunctionMap;

	std::string tag;
	FunctionMap funcOccurances;


	public:
	StringLiteralInfo(): funcOccurances(), tag() {
		}

	StringLiteralInfo(int tagNum) {
		tag = STRING_LITERAL_PREFIX + to_string(tagNum);
	}

	std::string getTag() const;

	int getFuncOccuranceNum() const;
	std::string getSummaryPrintout() const;

	protected:
	bool addFuncOccurance(SgFunctionDeclaration * func, SgStatement* stmt);

	friend class StringLiteralAnalysis;
	friend class StringLiteralAnalysisVisitor;
};

typedef std::map<std::string, StringLiteralInfo> LiteralMap;

class StringLiteralAnalysis {
	//Note: the memory allocated to the StringLiteralInfo held by LiteralMap must be manually freed
protected:
	int strCount;
	std::set<std::string> globalStrLiterals;
	LiteralMap strLiterals;
	SgProject *project;
public:
	StringLiteralAnalysis(SgProject *project): strCount(0), globalStrLiterals(), strLiterals(){
		this->project = project;
	}
	void runAnalysis();

	std::string getStringLiteralLabel(std::string literal);
	std::set<std::string> getStringLiterals();
	int getNumberOfStringLiterals();
	bool isGlobalStringLiteral(std::string str);
	StringLiteralInfo getStringLiteralInfo(std::string literal);
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
