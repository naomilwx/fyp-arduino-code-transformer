#include "stringLivenessAnalysis.h"
#include "stringValPropagation.h"

int main( int argc, char * argv[] ) {
  SgProject* project = frontend(argc,argv);

  initAnalysis(project);

  Dbg::init("String liveness test", ".", "lindex.html");

  StringValPropagation strValProp;
  strValProp.runAnalysis(project);
}
