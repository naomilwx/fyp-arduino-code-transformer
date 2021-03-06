#include "stringLiteralAnalysis.h"
#include "analysisCommon.h"

int main(int argc, char** argv){
	// Build the AST used by ROSE
	SgProject* project = frontend(argc, argv);

//	SgFile & fileInfo = project->get_file(0);
//	fileInfo.get_file_info()->display("file info");


	printf("begin analysis... \n");
	StringLiteralAnalysis analysis(project);

	analysis.runAnalysis();
	printf("%s\n", analysis.getAnalysisPrintout().c_str());

	return 0;
}
