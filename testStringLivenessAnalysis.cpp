#include "stringLivenessAnalysis.h"

int main( int argc, char * argv[] ) {
  SgProject* project = frontend(argc,argv);

  initAnalysis(project);

  Dbg::init("String liveness test", ".", "lindex.html");

  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();

  StringValPropagation strValProp;
  strValProp.runAnalysis(project);

  StringLivenessColouring livenessColouring(lanalysis.getStatementLiteralMap());
//  livenessColouring.runOverallAnalysis();
//
//  printAnalysis(&livenessColouring, false);
  printf("done colouring\n");
}
