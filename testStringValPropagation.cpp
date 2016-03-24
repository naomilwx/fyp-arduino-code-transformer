#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "codeSimplifier.h"

#include "analysisCommon.h"
//#include "liveDeadVarAnalysis.h"
#include "ctUtils.h"

void transformUnmodifiedStringVars(StringLiteralAnalysis *lanalysis, SgProject *project) {
	PointerAliasAnalysisDebugLevel = 0;
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

//  liveDeadAnalysisDebugLevel = 1;
  analysisDebugLevel = 1;

  LiveDeadVarsAnalysis ldva(project);
//       ldva.filter = gfilter; // the defaultFitler can provide the same semantics now
  UnstructuredPassInterDataflow ciipd_ldva(&ldva);
  //     assert (ciipd_ldva.filter == gfilter);
  ciipd_ldva.runAnalysis();



  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();

  PointerAliasAnalysisDebugLevel = 1;
  PointerAliasAnalysis pal(&ldva, project, lanalysis.getLiteralMap());
  pal.runAnalysis();
  printf("done\n");


}
