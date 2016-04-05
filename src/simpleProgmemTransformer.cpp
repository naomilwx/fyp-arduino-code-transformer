/*
 * simpleProgmemTransformer.cpp
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"

#include "analysisCommon.h"

#include "basicProgmemTransform.h"
#include "ctUtils.h"

//analysisDebugLevel must be set to 1, else return info is not stored...
int main( int argc, char * argv[] ) {
  SgProject* project = frontend(argc,argv);

  initAnalysis(project);
  cfgUtils::initCFGUtils(project);

  Dbg::init("progmem transform", "./debugprints", "index.html");

  analysisDebugLevel = 0;

  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();

  analysisDebugLevel = 1;
  PointerAliasAnalysisDebugLevel = 0;
  PointerAliasAnalysis pal(NULL, project, lanalysis.getLiteralMap());
  pal.runAnalysis();
  printf("done analysis\n");
  BasicProgmemTransform transformer(project, &pal, &lanalysis);
  transformer.runTransformation();
  printf("done transformation\n");
  backend(project);
}
