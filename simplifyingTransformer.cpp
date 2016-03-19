#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "codeSimplifier.h"

#include "analysisCommon.h"
#include "ctUtils.h"

void transformUnmodifiedStringVars(StringLiteralAnalysis *lanalysis, SgProject *project) {
	analysisDebugLevel = 1;
	PointerAliasAnalysisDebugLevel = 1;
	PointerAliasAnalysis pal(NULL, project, lanalysis->getLiteralMap());
	pal.runAnalysis();

	SimplifyOriginalCode soc(&pal, lanalysis, project);
	soc.transformUnmodifiedStringVars();
	printf("done first transform \n");
}


int main( int argc, char * argv[] ) {
  SgProject* project = frontend(argc,argv);

  initAnalysis(project);
  cfgUtils::initCFGUtils(project);
  Dbg::init("propagation test", "./proptest", "index.html");

  analysisDebugLevel = 0;



  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();

  transformUnmodifiedStringVars(&lanalysis, project);

  analysisDebugLevel = 1;
  PointerAliasAnalysisDebugLevel = 1;
  PointerAliasAnalysis pal(NULL, project, lanalysis.getLiteralMap());
  pal.runAnalysis();
//  printf("done\n");
//
  SimplifyOriginalCode soc(&pal, &lanalysis, project);
  soc.runTransformation();

  backend(project);
}
