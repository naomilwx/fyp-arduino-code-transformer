#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "codeSimplifier.h"

#include "analysisCommon.h"
#include "ctUtils.h"

#include "ssaUnfilteredCfg.h"
#include "defsAndUsesUnfilteredCfg.h"
#include "VariableRenaming.h"

//#include "DefUseAnalysis.h"
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

using namespace ssa_unfiltered_cfg;
typedef std::map<SgNode*, std::set<SgNode*> > DefUseChains;
//analysisDebugLevel must be set to 1, else return info is not stored...

void transformUnmodifiedStringVars(StringLiteralAnalysis *lanalysis, SgProject *project) {
	analysisDebugLevel = 1;
	PointerAliasAnalysisDebugLevel = 0;
	PointerAliasAnalysis pal(NULL, project, lanalysis->getLiteralMap());
	pal.runAnalysis();

	SimplifyOriginalCode soc(&pal, lanalysis, project);
	soc.transformUnmodifiedStringVars();
	printf("done first transform \n");
}


void generateDefUseChainsFromVariableRenaming(SgProject* project, DefUseChains& defUseChains){
    VariableRenaming varRenaming(project);
    varRenaming.run();

    const VariableRenaming::DefUseTable& useTable = varRenaming.getUseTable();
    foreach (const VariableRenaming::DefUseTable::value_type& usesOnNode, useTable){
        foreach (const VariableRenaming::TableEntry::value_type& entry, usesOnNode.second)
        {
            foreach (SgNode* node, entry.second)
            {
                defUseChains[node].insert(usesOnNode.first);
            }
        }
    }
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
  DefsAndUsesTraversal::CFGNodeToVarsMap defs;
  DefUseChains defUse;
  generateDefUseChainsFromVariableRenaming(project, defUse);

  //Setup analysis to gather string literal info
  StringLiteralAnalysis lanalysis(project);
  lanalysis.runAnalysis();

  transformUnmodifiedStringVars(&lanalysis, project);

  analysisDebugLevel = 1;
  PointerAliasAnalysisDebugLevel = 0;
  PointerAliasAnalysis pal(NULL, project, lanalysis.getLiteralMap());
  pal.runAnalysis();

  SimplifyOriginalCode soc(&pal, &lanalysis, project);
  soc.runGlobalTransformation(defUse);

  backend(project);
}
