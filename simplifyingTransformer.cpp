#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "codeSimplifier.h"

#include "analysisCommon.h"
#include "ctUtils.h"


int main( int argc, char * argv[] ) {
  SgProject* project = frontend(argc,argv);

  initAnalysis(project);
  cfgUtils::initCFGUtils(project);
  Dbg::init("propagation test", "./proptest", "index.html");

  analysisDebugLevel = 0;



  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();


  PointerAliasAnalysisDebugLevel = 0;
  PointerAliasAnalysis pal(NULL, project, lanalysis.getLiteralMap());
  pal.runAnalysis();
  printf("done\n");

  SimplifyOriginalCode soc(&pal, &lanalysis, project);
  soc.runTransformation();


}
