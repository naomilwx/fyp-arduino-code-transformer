#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "codeSimplifier.h"

#include "analysisCommon.h"
#include "ctUtils.h"

#include "ssaUnfilteredCfg.h"
#include "defsAndUsesUnfilteredCfg.h"
#include "DefUseAnalysis.h"

using namespace ssa_unfiltered_cfg;

void transformUnmodifiedStringVars(StringLiteralAnalysis *lanalysis, SgProject *project) {
	analysisDebugLevel = 1;
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

  analysisDebugLevel = 0;

  //Setup def use analysis for use later. This must be done before any changes to the current ast is made
  ssa_private::UniqueNameTraversal uniqueTrav(SageInterface::querySubTree<SgInitializedName > (project, V_SgInitializedName));
  uniqueTrav.traverse(project);
  SSA_UnfilteredCfg ssa(project);
  ssa.run();
  DefsAndUsesTraversal::CFGNodeToVarsMap defs;
  std::map<SgNode*, std::set<SgVarRefExp*> > defUse;
//  DefsAndUsesTraversal::CollectDefsAndUses(project, defs, defUse);

  //Setup analysis to gather string literal info
  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();

  transformUnmodifiedStringVars(&lanalysis, project);

  analysisDebugLevel = 1;
  PointerAliasAnalysisDebugLevel = 1;
  PointerAliasAnalysis pal(NULL, project, lanalysis.getLiteralMap());
  pal.runAnalysis();

  SimplifyOriginalCode soc(&pal, &lanalysis, project);
  soc.runGlobalTransformation(defUse);

  backend(project);
}
