#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "codeSimplifier.h"

#include "analysisCommon.h"
//#include "liveDeadVarAnalysis.h"
#include "ctUtils.h"


int main( int argc, char * argv[] ) {
  SgProject* project = frontend(argc,argv);

  initAnalysis(project);
  cfgUtils::initCFGUtils(project);
  Dbg::init("propagation test", "./proptest", "index.html");

//  liveDeadAnalysisDebugLevel = 1;
  analysisDebugLevel = 0;

  LiveDeadVarsAnalysis ldva(project);
//       ldva.filter = gfilter; // the defaultFitler can provide the same semantics now
  UnstructuredPassInterDataflow ciipd_ldva(&ldva);
  //     assert (ciipd_ldva.filter == gfilter);
  ciipd_ldva.runAnalysis();



  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();
  analysisDebugLevel = 1;
  PointerAliasAnalysisDebugLevel = 1;
  PointerAliasAnalysis pal(&ldva, project, lanalysis.getLiteralMap());
  pal.runAnalysis();
  printf("done\n");


}
