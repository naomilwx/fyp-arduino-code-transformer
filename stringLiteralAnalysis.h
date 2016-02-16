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
	typedef std::set<SgFunctionDeclaration *> FunctionSet;

	std::string tag;
	FunctionSet funcOccurances;

	public:
	StringLiteralInfo(): funcOccurances(), tag() {
		}

	StringLiteralInfo(int tagNum) {
		tag = STRING_LITERAL_PREFIX + to_string(tagNum);
	}

	std::string getTag() const;

	int getFuncOccuranceNum();
	
	protected:
	bool addFuncOccurance(SgFunctionDeclaration * func);

	friend class StringLiteralAnalysis;
};

class FunctionInfo {
	public:
		std::string functionName;
		SgFunctionDeclaration *declaration;
		FunctionInfo(): functionName(), declaration(){};
		FunctionInfo(SgFunctionDeclaration *func){
			declaration = func;
			functionName = func->get_name().getString();
		};
};

class StringLiteralAnalysis: public AstTopDownProcessing<FunctionInfo> {
	typedef std::map<std::string, StringLiteralInfo> LiteralMap;

protected:
	int strCount;
	std::set<std::string> globalStrLiterals;
	LiteralMap strLiterals;
public:
	virtual FunctionInfo evaluateInheritedAttribute(SgNode *node, FunctionInfo info);

	std::string getStringLiteralLabel(std::string literal);
	int getNumberOfStringLiterals();
};

//class StringLiteralAnalysis : public AstSimpleProcessing {
//protected:
//	int strCount;
//	std::set<std::string> globalLiterals;
//	std::map<std::string, std::string> strLiterals;
//public:
//	void visit(SgNode *n);
//	std::string getStringLiteralLabel(std::string literal);
//	int StringLiteralAnalysis::getNumberOfStringLiterals();
//};
