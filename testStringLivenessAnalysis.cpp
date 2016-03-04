#include "stringLivenessAnalysis.h"

int main( int argc, char * argv[] ) {
  SgProject* project = frontend(argc,argv);

  initAnalysis(project);

  Dbg::init("String liveness test", "./livetest", "lindex.html");

  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();

  StringValPropagation strValProp(project);
  strValProp.runAnalysis();


  StringLivenessColouring livenessColouring(project, lanalysis.getStatementLiteralMap());
  livenessColouring.runOverallAnalysis();

  printAnalysis(&livenessColouring, false);
  printf("done colouring\n");


  StringLivenessAnalysis stringLiveness(project, &strValProp, &livenessColouring);
  stringLiveness.runOverallAnalysis();
  printf("done liveness\n");
  stringLiveness.runAnnotation();

  printAnalysis(&stringLiveness, true);
  printf("done\n");

  backend(project);
}
