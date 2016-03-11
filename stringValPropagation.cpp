#include "stringValPropagation.h"
#include "analysisCommon.h"
#include "ctUtils.h"
int debugLevel = 2;

StringValPropagationTransfer::StringValPropagationTransfer(const Function &func, const DataflowNode &n, NodeState &state, const std::vector<Lattice *>& dfInfo) : VariableStateTransfer<StringValLattice>(func, n, state, dfInfo, debugLevel) {
}

void StringValPropagationTransfer::visit(SgStringVal *n) {
//	printf("visited...string...\n");
	StringValLattice* lattice = getLattice(n);
	lattice->addPossibleVal(n->get_value());
}

void StringValPropagationTransfer::visit(SgPntrArrRefExp *n) {
	//TODO: this is wrong. array ref on the rhs should not be marked
	StringValLattice* lattice = getLattice(n->get_lhs_operand());
	modified = lattice->setLevel(StringValLattice::TOP) || modified;

}

void StringValPropagationTransfer::visit(SgInitializedName *n){
	VariableStateTransfer<StringValLattice>::visit(n);
		StringValLattice* lattice = getLattice(n);
		if(lattice && lattice->getLevel() == StringValLattice::BOTTOM) {
			SgType *type = n->get_type();
			if(isSgTypeChar(type)) {
				return;
			}
			if(isArduinoStringType(type)) {
				modified = lattice->setLevel(StringValLattice::TOP) || modified;
			} else if(isSgTypeChar(type->findBaseType())) {
				modified = lattice->setLevel(StringValLattice::INITIALISED) || modified;
			}

		}
}
//void StringValPropagationTransfer::visit(SgAssignOp *n){
//	VariableStateTransfer<StringValLattice>::visit(n);
//	SgExpression *rhs = n->get_rhs_operand ();
//	StringValLattice* lattice = getLattice(rhs);
//	if(lattice->getLevel() == StringValLattice::BOTTOM) {
//		lattice->setLevel(StringValLattice::TOP);
//	}
//}
void StringValPropagationTransfer::visit(SgFunctionCallExp *n){
	SgExpression *funcRef = getFunctionRef(n);
	SgExpressionPtrList params = n->get_args()->get_expressions();
	if(funcRef != NULL) {
		SgFunctionType *funcType = dynamic_cast<SgFunctionType *>(funcRef->get_type());
		SgTypePtrList fArgs = funcType->get_arguments();
		int argIdx = 0;
		for(auto &fArg : fArgs) {
			if(fArg->containsInternalTypes() && isConstantType(fArg) == false) {
				StringValLattice* lattice = getLattice(params[argIdx]);
				modified = lattice->setLevel(StringValLattice::TOP) || modified;
			}
			argIdx++;
		}
	}

}

bool StringValPropagationTransfer::finish() {
	return modified;
}

//StringValPropagation Implementation

void StringValPropagation::genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts) {
	std::map<varID, Lattice*> m;
	initLattices.push_back(new FiniteVarsExprsProductLattice((Lattice *) new StringValLattice(), m, (Lattice *)NULL, NULL, n, state));
}

bool StringValPropagation::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo) {
	return false;
}

boost::shared_ptr<IntraDFTransferVisitor> StringValPropagation::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo){
	return boost::shared_ptr<IntraDFTransferVisitor>(new StringValPropagationTransfer(func, n, state, dfInfo));
}


StringValLattice *StringValPropagation::getValLattice(SgNode *n, SgNode *var){
	FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(*(NodeState::getLatticeBelow(this, n, 0).begin()));
	return dynamic_cast<StringValLattice *>(lat->getVarLattice(varID(var)));
}

StringValLattice *StringValPropagation::getValLattice(NodeState *s, varID var) {
	FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(*(s->getLatticeBelow(this).begin()));
	return dynamic_cast<StringValLattice *>(lat->getVarLattice(var));
}
bool StringValPropagation::isModifiedStringRef(SgFunctionDefinition *def, SgVarRefExp *var) {
	DataflowNode n = cfgUtils::getFuncEndCFG(def, filter);
	NodeState *state = NodeState::getNodeState(n, n.getIndex());
	FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(*(state->getLatticeBelow(this).begin()));
	return dynamic_cast<StringValLattice *>(lat->getVarLattice(varID(var)))->getLevel() == StringValLattice::TOP;
}

void StringValPropagation::runAnalysis() {
	 SgIncidenceDirectedGraph *graph = buildProjectCallGraph(project);
	 ContextInsensitiveInterProceduralDataflow inter(this, graph);
	 inter.runAnalysis();
}


// **********************************************************************
//                     PointerAliasAnalysisTransfer
// **********************************************************************

/*PointerAliasAnalysisTransfer::PointerAliasAnalysisTransfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
   : IntraDFTransferVisitor(func, n, state, dfInfo)
{}
*/


int PointerAliasAnalysisDebugLevel = 1;

PointerAliasAnalysisTransfer::PointerAliasAnalysisTransfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
   : VariableStateTransfer<PointerAliasLattice>(func, n, state, dfInfo, PointerAliasAnalysisDebugLevel)
{}



//Transfer function for Assign operations. 
//Calculates the aliasDerefCount for left and right side of AssignOP expression and updates lattice with alias information
void PointerAliasAnalysisTransfer::visit(SgAssignOp *sgn)
{
      ROSE_ASSERT(sgn != NULL);
      PointerAliasLattice *resLat = getLattice(sgn);

      SgExpression *lhs = NULL;
      SgExpression *rhs = NULL;
      aliasDerefCount leftARNode, rightARNode;

      SageInterface::isAssignmentStatement(sgn,&lhs, &rhs);
      Dbg::dbg << "AssignOP Stement"<<lhs->variantT()<<"and"<<rhs->variantT()<<"\n"; 
      processLHS(lhs,leftARNode);
      processRHS(rhs,rightARNode);

      //Establish the per CFG-node alias relations
      if((leftARNode.var !=NULL) && (rightARNode.var !=NULL))
        resLat->setAliasRelation(make_pair(leftARNode,rightARNode)); 
        
      //Update the aliasedVariables(Compact Representation Graph)
      updateAliases(resLat->getAliasRelations(),1);
}



//Transfer function for Function Call Expressions. 
//Gets the lattice of the function call expression and updates lattice with alias information
void PointerAliasAnalysisTransfer::visit(SgFunctionCallExp *sgn)
{
    PointerAliasLattice *resLat = getLattice(sgn);
    updateAliases(resLat->getAliasRelations(),1);
}




//Transfer function for AssignInitializer operations. 
//Calculates the aliasDerefCount for left and right side of AssignInitializer expression and updates lattice with alias information
void PointerAliasAnalysisTransfer::visit(SgAssignInitializer *sgn)
{
    
    SgAssignInitializer *assgn_i = isSgAssignInitializer(sgn);
    assert(assgn_i != NULL);
    PointerAliasLattice *resLat = getLattice(sgn);

    SgExpression *lhs = static_cast<SgExpression *> (assgn_i->get_parent());
    SgExpression *rhs = assgn_i->get_operand();
    aliasDerefCount leftARNode, rightARNode;

    processLHS(lhs,leftARNode);
    processRHS(rhs,rightARNode);

    //Establish the per CFG-node alias relations
    if((leftARNode.var !=NULL) && (rightARNode.var !=NULL))
        resLat->setAliasRelation(make_pair(leftARNode,rightARNode)); 
      
    //Update the Aliases(Compact Representation Graph)
    updateAliases(resLat->getAliasRelations(),1);
}




//Transfer function for Constructor Initalizers. 
//Gets the lattice of the constructor initializer and updates lattice with alias information
void PointerAliasAnalysisTransfer::visit(SgConstructorInitializer *sgn)
{
    PointerAliasLattice *resLat = getLattice(sgn);
    updateAliases(resLat->getAliasRelations(),1);
}


bool PointerAliasAnalysisTransfer::finish()
{
  return modified;
}



//Update the Aliases (compact representation graph)
/*
    aliasRelations : Set of Alias Relations that hold at the CFG-node
    isMust : Is Must Alias or not
    - For each alias relation pair compute aliases at the left and right expressions in the pair
    - If is must alias, remove existing aliases of left variable lattice
    - Add a alias between each left variable and all their right variable aliases 
*/
bool PointerAliasAnalysisTransfer::updateAliases(set< std::pair<aliasDerefCount, aliasDerefCount> > aliasRelations, int isMust)
{
    bool modified = false;
    PointerAliasLattice *toLat, *fromLat;
    set <varID>  rightResult, leftResult;

    for(set< std::pair<aliasDerefCount, aliasDerefCount> >::iterator alRel = aliasRelations.begin();
            alRel!=aliasRelations.end();alRel++ )
    {
        computeAliases(getLattice((alRel->first).vID), (alRel->first).vID, (alRel->first).derefLevel, leftResult);
        computeAliases(getLattice((alRel->second).vID), (alRel->second).vID, (alRel->second).derefLevel+1 , rightResult); 
  	printf("lhs vid %s\n", (alRel->first).vID.str().c_str());
	printf("rhs vid %s\n", (alRel->second).vID.str().c_str());
	Dbg::dbg<<"LEFT ALIAS SIZE:" <<leftResult.size() <<"RIGHT ALIAS SIZE :"<<rightResult.size(); 

        if(isMust){
            for(set<varID>::iterator leftVar = leftResult.begin(); leftVar != leftResult.end(); leftVar++ )
            {
                toLat = getLattice(*leftVar);
                //if((toLat->getAliases()).size()==1)
                //{   
                    toLat->clearAliasedVariables();
                    modified = true; 
                //}
            }
        }
       
        for(set<varID>::iterator leftVar = leftResult.begin(); leftVar != leftResult.end(); leftVar++ ) {
            toLat = getLattice(*leftVar);
            for(set<varID>::iterator rightVar = rightResult.begin(); rightVar != rightResult.end(); rightVar++ ) {
               toLat->setAliasedVariables(*rightVar); 
               modified = true; 
            }
        }  
    }  
//	printf("updated alias\n");
return modified; 
}


//Compute Aliases by recursively walking through the compact representation of per-variable lattices
/*
    Consider 
            int **x;
            int *p,*q;
            int a;
            p = &a;
            q = p;
            x = &p;
    
    With per variable lattices like so:
            p: {a}
            q: {a}
            x: {p}
    To compute aliases for a pointer say 'x' with a deref count =2, we recursively travserse through the pervariableLattices sets to compute its aliases.
    Ex :  computeAliases('x',2,result) -->computeAliases('p', 1, result)  --> computeAliases('a',0,result) --> result = {a}   
*/
void PointerAliasAnalysisTransfer::computeAliases(PointerAliasLattice *lat, varID var, int derefLevel, set<varID> &result)
{
   if(derefLevel==0)
    result.insert(var);
   else
    {
        set<varID> outS = lat->getAliasedVariables();
        for(set<varID>::iterator outVar = outS.begin(); outVar != outS.end(); outVar++)
        {
            computeAliases(getLattice(*outVar),*outVar,derefLevel-1,result);
        }
    }
}

 
//- process left hand side of expressions(node) and calculate the dereference count for pointer and reference variables
//- The newly found alias relation is placed in the arNode. 
//- Alias relations are established only for pointer and references.
void PointerAliasAnalysisTransfer::processLHS(SgNode *node,struct aliasDerefCount &arNode) {
    if(node == NULL)
        return;

    SgVariableSymbol *sym = NULL;
    SgVarRefExp *var_exp;
    varID var;
    int derefLevel = 0;
    
   
    switch (node->variantT()) {
        case V_SgInitializedName:
        {
            SgInitializedName *init_exp = isSgInitializedName(node);
            ROSE_ASSERT(init_exp != NULL);
            sym = static_cast<SgVariableSymbol *>(init_exp->get_symbol_from_symbol_table());
            //var = varID(init_exp);
            var = SgExpr2Var(init_exp->get_initializer());
        }
        break;

        case V_SgVarRefExp:
        {
             var_exp = isSgVarRefExp(node);
             ROSE_ASSERT(var_exp != NULL);
             sym = var_exp->get_symbol();
             var = SgExpr2Var(var_exp);
        }
        break;

        case V_SgPointerDerefExp:
        {
            SgPointerDerefExp *ptr_exp = isSgPointerDerefExp(node);
            ROSE_ASSERT(ptr_exp != NULL);
            processLHS(ptr_exp->get_operand(), arNode);
            derefLevel++;
            arNode.derefLevel += derefLevel;
            return;
        }
        break;
        case V_SgDotExp:
        case V_SgArrowExp:
        {
            SgBinaryOp *bin_exp = isSgBinaryOp(node);
            ROSE_ASSERT(bin_exp != NULL);
            processLHS(bin_exp->get_rhs_operand(), arNode);
            return;
        }
        break;

        default:
            sym = NULL;
    };


    //Maintain alias relation for Pointer/Reference types only
    if(sym != NULL &&
            (SageInterface::isPointerType(sym->get_type()) == false && SageInterface::isReferenceType(sym->get_type())==false) )
    {
            sym = NULL;
    } 

    arNode.var = sym;
    arNode.vID = var;
    arNode.derefLevel = derefLevel;
}


//- Process aliases for Right hand side of expression and calculate derefernce count for pointer and reference variables
//- The newly found alias relation is placed in the arNode. 
//- Alias relations are established only for pointer and references.
void PointerAliasAnalysisTransfer::processRHS(SgNode *node, struct aliasDerefCount &arNode) {
    if(node == NULL)
        return;

    SgVariableSymbol *sym = NULL;
    SgVarRefExp *var_exp;
    varID var;
    int derefLevel = 0;
    static int new_index;
    static map<SgExpression*, SgVariableSymbol *> new_variables;

    switch (node->variantT()) {

        // = a
        case V_SgVarRefExp:
        {
            var_exp = isSgVarRefExp(node);
            ROSE_ASSERT(var_exp != NULL);
            sym = var_exp->get_symbol();
            var = SgExpr2Var(var_exp);
        }
        break;

        // = *a or = **a or = ***a  etc
        case V_SgPointerDerefExp:
        {
            SgPointerDerefExp *ptr_exp = isSgPointerDerefExp(node);
            ROSE_ASSERT(ptr_exp != NULL);
            processRHS(ptr_exp->get_operand(), arNode);
            derefLevel++;
            arNode.derefLevel += derefLevel;
            return;
        }
        break;
        //  = &b or = &&b(supported in C++Ox) etc
        case V_SgAddressOfOp:
        {
            SgAddressOfOp *add_exp = isSgAddressOfOp(node);
            ROSE_ASSERT(add_exp != NULL);
            processRHS(add_exp->get_operand(), arNode);
            sym = arNode.var;
            var = arNode.vID;
            derefLevel = arNode.derefLevel-1;
        }
       break;
       // a.b or a->b or this->b
        case V_SgDotExp:
        case V_SgArrowExp:
        {
            SgBinaryOp *bin_exp = isSgBinaryOp(node);
            ROSE_ASSERT(bin_exp != NULL);
            processRHS(bin_exp->get_rhs_operand(), arNode);
            return;
        }
        break;
        // Case where *a = *b = *c ...etc
       case V_SgAssignOp:
       {
            SgExpression *lhs = NULL;
            SgExpression *rhs = NULL;
            if(SageInterface::isAssignmentStatement(node,&lhs, &rhs)){
                ROSE_ASSERT(rhs != NULL);
                processRHS(rhs, arNode);
                return;
            }
       }
       break;

       case V_SgCastExp:
       {
           SgCastExp *cast_exp = isSgCastExp(node);
           processRHS(cast_exp->get_operand(), arNode);
           return;
       }
       break;
       // C *c = new C or
       // c = new C
       //return new C
       /*
           We follow the rule that every memory location has a unique name.
           To assign a unique name for 'new Class()' expression we assign "__tmp_Mem_x" to identify it using a SgVariableSymbol name
           The AST is updated to reflect the new variable Declaration
       */
       case V_SgNewExp:
       {
        SgNewExp *new_exp = isSgNewExp(node);
        if(new_variables.count(new_exp) == 0){

           SgScopeStatement *scope = NULL;
           SgNode *parent=new_exp->get_parent();
           SgStatement *stmt = NULL;

           // find the nearest parent which is a statement
           while(!(stmt = isSgStatement(parent)))
                parent = parent->get_parent();

           scope = stmt->get_scope();
           SgType *type = new_exp->get_type()->dereference();
           std::stringstream ss;
           ss << "__tmp_Mem__" << new_index;
           std::string name;
           ss >> name;
           SgName var_name = name;
           new_index++;
           assert(scope != NULL);

           // Create a temporary variable so that every memory location has an assigned name
           SageBuilder::buildVariableDeclaration_nfi(var_name, type, NULL, scope);

           //SageInterface::prependStatement(var_decl, scope);
           sym = scope->lookup_variable_symbol(var_name);
           ROSE_ASSERT(sym != NULL);
           new_variables[new_exp] = sym;
        }
        else
            sym = new_variables[new_exp];
           
        var =  SgExpr2Var(new_exp);
        derefLevel = derefLevel - 1;
       }
       break;
       default:
            sym = NULL;
    }

    arNode.derefLevel = derefLevel;
    arNode.var = sym;
    arNode.vID = var;
}



// **********************************************************************
//                     PointerAliasAnalysis
// **********************************************************************
void PointerAliasAnalysis::genInitState(const Function& func, const DataflowNode& n, const NodeState& state,std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts)
{
     map<varID, Lattice*> emptyM;
     initLattices.push_back(new FiniteVarsExprsProductLattice((Lattice*) new PointerAliasLattice(), emptyM, (Lattice*)NULL,NULL, n, state) );

}

PointerAliasAnalysis::PointerAliasAnalysis(LiveDeadVarsAnalysis* ldva)   
{
     this->ldva = ldva;   
}


bool PointerAliasAnalysis::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
{
    assert(0);
    return false;
}

boost::shared_ptr<IntraDFTransferVisitor>
PointerAliasAnalysis::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
{
    return boost::shared_ptr<IntraDFTransferVisitor>(new PointerAliasAnalysisTransfer(func, n, state, dfInfo));
}
