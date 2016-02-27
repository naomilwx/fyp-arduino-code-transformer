#include "stringValPropagation.h"

#include "analysisCommon.h"
#include "liveDeadVarAnalysis.h"
#include "ctUtils.h"

bool gfilter (CFGNode cfgn)
{
  SgNode *node = cfgn.getNode();
//  printf("class %s\n", node->class_name().c_str());

  if(isSgFunctionDeclaration(node)){
	printf("function: %s\n", isSgFunctionDeclaration(node)->get_name().getString().c_str());
// 	return true; 
 }
//else {
//    printf("function: %s\n", isSgFunctionDeclaration(node)->get_name().getString().c_str());
 //}
  return cfgn.isInteresting();
//  switch (node->variantT())
//  {
//    //Keep the last index for initialized names. This way the def of the variable doesn't propagate to its assign initializer.
//    case V_SgInitializedName:
//      return (cfgn == node->cfgForEnd());
//
//    // For function calls, we only keep the last node. The function is actually called after all its parameters  are evaluated.
//    case V_SgFunctionCallExp:
//      return (cfgn == node->cfgForEnd());
//
//   //For basic blocks and other "container" nodes, keep the node that appears before the contents are executed
//    case V_SgBasicBlock:
//    case V_SgExprStatement:
//    case V_SgCommaOpExp:
//      return (cfgn == node->cfgForBeginning());
//   // Must have a default case: return interesting CFGNode by default in this example
//    default:
//      return cfgn.isInteresting();
//
//  }
}


int main( int argc, char * argv[] ) {
  SgProject* project = frontend(argc,argv);

  initAnalysis(project);
  Dbg::init("propagation test", ".", "index.html");

//  DefinedFunctionCollector definedFuncsCollector;
//  definedFuncsCollector.traverseInputFiles(project, preorder);
//  definedFuncsCollector.printDefinedFunctions();

  liveDeadAnalysisDebugLevel = 1;
  analysisDebugLevel = 1;

  //LiveDeadVarsAnalysis ldva(project);
//       ldva.filter = gfilter; // the defaultFitler can provide the same semantics now
 // UnstructuredPassInterDataflow ciipd_ldva(&ldva);
  //     assert (ciipd_ldva.filter == gfilter);
  //ciipd_ldva.runAnalysis();

//  CallGraphBuilder cgb(project);
//  cgb.buildCallGraph(definedFuncsFilter(definedFuncsCollector.getDefinedFuncs()));
//  SgIncidenceDirectedGraph *graph = cgb.getGraph();

  StringValPropagation strValProp;
//
//  ContextInsensitiveInterProceduralDataflow inter(&strValProp, graph);
//  inter.runAnalysis();

  strValProp.runAnalysis(project);
  printAnalysis(&strValProp, false);

  printf("done\n");
}