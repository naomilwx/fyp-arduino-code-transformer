#include "stringLiteralAnalysis.h"

int main(int argc, char** argv){
	// Build the AST used by ROSE
	SgProject* project = frontend(argc, argv);

	StringLiteralAnalysis analysis;

}
