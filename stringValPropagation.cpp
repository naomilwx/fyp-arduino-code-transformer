#include "boost/mem_fn.hpp"

#include "stringValPropagation.h"
#include "analysisCommon.h"
#include "ctUtils.h"
using std::auto_ptr;

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

PointerAliasAnalysisTransfer::PointerAliasAnalysisTransfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo, LiteralMap *map)
	: VariableStateTransfer<PointerAliasLattice>(func, n, state, dfInfo, PointerAliasAnalysisDebugLevel){
		this->literalMap = map;
	}



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

	//      if(isSgPntrArrRefExp(lhs)){
	//    	  printf("array: %s %s\n", leftARNode.vID.str().c_str(), lhs->unparseToString().c_str());
	//      }

	set <varID>  result;
	PointerAliasLattice *lhsLat;
	computeAliases(getLattice(leftARNode.vID), leftARNode.vID, leftARNode.derefLevel, result);
	for(auto &var: result) {
		lhsLat = getLattice(var);
		if(lhsLat) {
			if(lhsLat->getState() == PointerAliasLattice::BOTTOM) {
				lhsLat->setState(PointerAliasLattice::INITIALIZED);
			} else if(isSgPntrArrRefExp(lhs)){
				lhsLat->setState(PointerAliasLattice::MODIFIED);
			} else{
				lhsLat->setState(PointerAliasLattice::REASSIGNED);
			}
		}

	}

	//Establish the per CFG-node alias relations
	if((leftARNode.var !=NULL) && (rightARNode.var !=NULL))
		resLat->setAliasRelation(make_pair(leftARNode,rightARNode)); 
	//TODO: handle array
	//Update the aliasedVariables(Compact Representation Graph)
	if(isSgPntrArrRefExp(lhs)){
		updateAliases(resLat->getAliasRelations(),0);
	} else { 
		updateAliases(resLat->getAliasRelations(),1);
	}
}



//Transfer function for Function Call Expressions. 
//Gets the lattice of the function call expression and updates lattice with alias information
void PointerAliasAnalysisTransfer::visit(SgFunctionCallExp *sgn) {
	//TODO: handle arguments modified by function
	PointerAliasLattice *resLat = getLattice(sgn);
	updateAliases(resLat->getAliasRelations(),1);
}

void PointerAliasAnalysisTransfer::visit(SgFunctionDefinition *fdef) {
	SgFunctionParameterList *params = isSgFunctionDeclaration(fdef->get_parent())->get_parameterList();
	SgInitializedNamePtrList args = params->get_args();
	int index = 0;

	SgScopeStatement *scope = fdef->get_scope();

	for(SgInitializedName *arg: args) {
		aliasDerefCount left, right;

		processLHS(arg, left);

		if(SageInterface::isReferenceType(arg->get_type())){
			left.derefLevel += 1;
		}
		PointerAliasLattice* lhsLat =  getLattice(left.vID);

		if(lhsLat){
			//		printf("arglat\n");
			lhsLat->setState(PointerAliasLattice::INITIALIZED);
		}

		processParam(index, scope, arg, right);	

		if(lhsLat && (left.var != NULL) && right.var != NULL) {
			lhsLat->setAliasRelation(make_pair(left,right));
			updateAliases(lhsLat->getAliasRelations(), 1);
		}
	}
}


//Transfer function for AssignInitializer operations. 
//Calculates the aliasDerefCount for left and right side of AssignInitializer expression and updates lattice with alias information
void PointerAliasAnalysisTransfer::visit(SgAssignInitializer *sgn) {
	SgAssignInitializer *assgn_i = isSgAssignInitializer(sgn);
	assert(assgn_i != NULL);
	PointerAliasLattice *resLat = getLattice(sgn);

	SgExpression *lhs = static_cast<SgExpression *> (assgn_i->get_parent());
	SgExpression *rhs = assgn_i->get_operand();
	aliasDerefCount leftARNode, rightARNode;

	processLHS(lhs,leftARNode);
	processRHS(rhs,rightARNode);


	PointerAliasLattice* lhsLat =  getLattice(leftARNode.vID);
	if(lhsLat){
		lhsLat->setState(PointerAliasLattice::INITIALIZED);
	}

	//Establish the per CFG-node alias relations
	if((leftARNode.var !=NULL) && (rightARNode.var !=NULL))
		resLat->setAliasRelation(make_pair(leftARNode,rightARNode)); 

	//Update the Aliases(Compact Representation Graph)
	updateAliases(resLat->getAliasRelations(),1);
}


void PointerAliasAnalysisTransfer::visit(SgAggregateInitializer *sgn) {
	PointerAliasLattice *resLat = getLattice(sgn);

	SgExpression *lhs = static_cast<SgExpression *>(sgn->get_parent());
	//printf("lhs %s\n", lhs->class_name().c_str());
	aliasDerefCount leftArNode;
	processLHS(lhs, leftArNode);
	//printf("start\n");
	PointerAliasLattice* lhsLat =  getLattice(leftArNode.vID);
	if(lhsLat){
		lhsLat->setState(PointerAliasLattice::INITIALIZED);
	}

	if(leftArNode.var != NULL){
		//printf("getting rhs\n");
		SgExprListExp *lst = sgn->get_initializers();
		for(auto &exp: lst->get_expressions()) {
			aliasDerefCount rightArNode;
			processRHS(exp, rightArNode);
			if(rightArNode.var != NULL){
				resLat->setAliasRelation(make_pair(leftArNode, rightArNode));
				//			printf("added\n");
			}
		}
	}
	updateAliases(resLat->getAliasRelations(), 1);
	//printf("finish\n");
}

//Transfer function for Constructor Initalizers. 
//Gets the lattice of the constructor initializer and updates lattice with alias information
void PointerAliasAnalysisTransfer::visit(SgConstructorInitializer *sgn)
{
	SgExpression *lhs = static_cast<SgExpression *>(sgn->get_parent());
	aliasDerefCount leftNode;
	processLHS(lhs, leftNode);

	PointerAliasLattice* lhsLat =  getLattice(leftNode.vID);
	if(lhsLat){
		lhsLat->setState(PointerAliasLattice::INITIALIZED);
	}

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
		//printf("lhs vid: %s %d\n", (alRel->first).vID.str().c_str(), (alRel->first).derefLevel);
		//printf("rhs vid: %s %d\n", (alRel->second).vID.str().c_str(), (alRel->second).derefLevel+1);
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
			//	    if(rightResult.size() == 0) {
			//		toLat->setAliasedVariables((alRel->second).vID);
			//           }
			//printf("lhs %s %d\n", (*leftVar).str().c_str(), rightResult.size());
			for(set<varID>::iterator rightVar = rightResult.begin(); rightVar != rightResult.end(); rightVar++ ) {
				toLat->setAliasedVariables(*rightVar); 
				modified = true; 
				//		printf("set alias %s\n", (*leftVar).name.c_str());
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

void PointerAliasAnalysisTransfer::processParam(int index, SgScopeStatement *scope, SgInitializedName *param, struct aliasDerefCount &arNode){
	SgType *type = param->get_type();
	std::stringstream ss;
	ss << "__function_param_"<< index;
	std::string name = ss.str();
	SgName var_name = name;
	SgVariableDeclaration *dec = SageBuilder::buildVariableDeclaration_nfi(var_name, type, NULL, scope);   
	arNode.var = scope->lookup_variable_symbol(var_name);
	arNode.vID = varID(name);
	arNode.derefLevel = -1;	
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

	bool isArrayOfPtr = false;

	switch (node->variantT()) {
		case V_SgInitializedName:
			{
				SgInitializedName *init_exp = isSgInitializedName(node);
				ROSE_ASSERT(init_exp != NULL);
				sym = static_cast<SgVariableSymbol *>(init_exp->get_symbol_from_symbol_table());
				//var = varID(init_exp);
				SgType *type = SageInterface::getElementType(init_exp->get_type());
				if(type != NULL && SageInterface::isPointerType(type)){	
					isArrayOfPtr = true;
				}	
				if(init_exp->get_initializer()){
					var = SgExpr2Var(init_exp->get_initializer());
				} else {
					var = varID(init_exp);
				}
				printf("sym: %s %s %s\n", sym->get_name().str(), var.str().c_str(), init_exp->get_type()->class_name().c_str());
			}
			break;

		case V_SgVarRefExp:
			{
				var_exp = isSgVarRefExp(node);
				ROSE_ASSERT(var_exp != NULL);
				sym = var_exp->get_symbol();
				var = SgExpr2Var(var_exp);
				SgType *type = SageInterface::getElementType(var_exp->get_type());
				if(type != NULL && SageInterface::isPointerType(type)){
					isArrayOfPtr = true;
				}
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
		case V_SgPntrArrRefExp: {
						SgPntrArrRefExp *arrRefExp = isSgPntrArrRefExp(node);
						processLHS(arrRefExp->get_lhs_operand(), arNode);
						//		printf("type %s %s\n",arrRefExp->get_type()->class_name().c_str(), (arNode.var)?arNode.var->get_type()->class_name().c_str():"");
						if(arNode.var) {
							if(SageInterface::isPointerType(arNode.var->get_type())) {
								arNode.derefLevel += 1;
							}
							if(SageInterface::isPointerType(arrRefExp->get_type()) == false) {
								arNode.var = NULL;
							}
						}
						return;
					}
					break;
		default:
					sym = NULL;
	};


	//Maintain alias relation for Pointer/Reference types only
	if(isArrayOfPtr == false && sym != NULL &&
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
				if(isSgArrayType(var_exp->get_type()) && derefLevel == 0){
					derefLevel = -1;
				}
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
		case V_SgStringVal:              
			{
				derefLevel = derefLevel - 1;
				SgVariableDeclaration *decl = (*literalMap)[(isSgStringVal(node)->get_value())].getPlaceholder();
				SgInitializedName *name = decl->get_variables().at(0);
				sym = isSgVariableSymbol(name->get_symbol_from_symbol_table());
				var = varID(name);
				//	   printf("string: %s %s\n", isSgStringVal(node)->get_value().c_str(), sym->get_name().str());
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
void PointerAliasAnalysis::genInitState(const Function& func, const DataflowNode& n, const NodeState& state,std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts){
	map<varID, Lattice*> emptyM;
	initLattices.push_back(new FiniteVarsExprsProductLattice((Lattice*) new PointerAliasLattice(), emptyM, (Lattice*)NULL,NULL, n, state) );
	//	printf("init node %s\n", n.getNode()->class_name().c_str());
}

PointerAliasAnalysis::PointerAliasAnalysis(LiveDeadVarsAnalysis* ldva, SgProject *project, LiteralMap *map)   
{
	this->ldva = ldva;   
	this->literalMap = map;
	this->project = project;
	this->filter = PointerAliasAnalysis::paaFilter;
}


bool PointerAliasAnalysis::transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
{
	assert(0);
	return false;
}

	boost::shared_ptr<IntraDFTransferVisitor>
PointerAliasAnalysis::getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo)
{
	return boost::shared_ptr<IntraDFTransferVisitor>(new PointerAliasAnalysisTransfer(func, n, state, dfInfo, literalMap));
}

PointerAliasLattice *PointerAliasAnalysis::getAliasLattice(NodeState *s, varID var){
	FiniteVarsExprsProductLattice *lat = dynamic_cast<FiniteVarsExprsProductLattice *>(*(s->getLatticeBelow(this).begin()));
	return dynamic_cast<PointerAliasLattice *>(lat->getVarLattice(var)); 
}

bool PointerAliasAnalysis::doAnalysis(const Function& func, NodeState* fState, bool analyzeDueToCallers, set<Function> calleesUpdated) {
	ostringstream funcNameStr; funcNameStr << "Function "<<func.get_name().getString()<<"()";
	bool firstVisit = visited.find(func) == visited.end();
	// Initialize the lattices used by this analysis, if this is the first time the analysis visits this function
	if(firstVisit)    {
		InitDataflowState ids(this);
		ids.runAnalysis(func, fState);
		visited.insert(func);
	}


	auto_ptr<VirtualCFG::dataflow> workList(getInitialWorklist(func, firstVisit, analyzeDueToCallers, calleesUpdated, fState));

	VirtualCFG::dataflow &it = *workList;
	VirtualCFG::iterator itEnd = VirtualCFG::dataflow::end();

	for(; it != itEnd; it++){
		DataflowNode n = *it;
		SgNode* sgn = n.getNode();
		ostringstream nodeNameStr;
		nodeNameStr << "Current Node "<<sgn<<"["<<sgn->class_name()<<" | "<<Dbg::escape(sgn->unparseToString())<<" | "<<n.getIndex()<<"]";
		if(analysisDebugLevel>=1){
			Dbg::enterFunc(nodeNameStr.str());
		}
		bool modified = false;

		// the number of NodeStates associated with the given dataflow node
		int numStates=NodeState::numNodeStates(n);
		ROSE_ASSERT(numStates == 1);
		// the NodeStates themselves
		const vector<NodeState*> nodeStates = NodeState::getNodeStates(n);	
		int i=0;
		NodeState* state = NULL;
		for(vector<NodeState*>::const_iterator itS = nodeStates.begin(); itS!=nodeStates.end(); ){
			state = *itS;

			// reset the modified state, since only the last NodeState's change matters

			// =================== Copy incoming lattices to outgoing lattices ===================
			const vector<Lattice*> dfInfoAnte = getLatticeAnte(state);
			const vector<Lattice*> dfInfoPost = getLatticePost(state);

			// Overwrite the Lattices below this node with the lattices above this node.
			// The transfer function will then operate on these Lattices to produce the
			// correct state below this node.

			vector<Lattice*>::const_iterator itA, itP;
			int j=0;
			for(itA  = dfInfoAnte.begin(), itP  = dfInfoPost.begin();
					itA != dfInfoAnte.end() && itP != dfInfoPost.end(); 
					itA++, itP++, j++)
			{
				if(analysisDebugLevel>=1){
					Dbg::dbg << " ==================================  "<<endl;
					Dbg::dbg << " Copy incoming lattice to outgoing lattice: "<<endl;
					Dbg::dbg << "  Incoming/Above Lattice "<<j<<": \n        "<<(*itA)->str("            ")<<endl;
					Dbg::dbg << "  Outgoing/Below Lattice before copying "<<j<<": \n        "<<(*itP)->str("            ")<<endl;
				}
				(*itP)->copy(*itA);

				if(analysisDebugLevel>=1){
					Dbg::dbg << "  Outgoing/Below Lattice after copying "<<j<<": \n        "<<(*itP)->str("            ")<<endl;
				}
			}

			// =================== TRANSFER FUNCTION ===================

			if(analysisDebugLevel>=1){
				Dbg::dbg << " ==================================  "<<endl;
				Dbg::dbg << "  Transferring the outgoing  Lattice ... "<<endl;
			}

			//if this is a call site, call transfer function of the associated interprocedural analysis
			//TODO: modify this...
			if (isSgFunctionCallExp(sgn))
				transferFunctionCall(func, n, state);

			boost::shared_ptr<IntraDFTransferVisitor> transferVisitor = getTransferVisitor(func, n, *state, dfInfoPost);

			sgn->accept(*transferVisitor);
			modified = transferVisitor->finish() || modified;


			// =================== TRANSFER FUNCTION ===================
			if(analysisDebugLevel>=1)
			{
				j=0;
				for(itP = dfInfoPost.begin();
						itP != dfInfoPost.end(); itP++, j++)
				{
					Dbg::dbg << "    Transferred: outgoing Lattice "<<j<<": \n        "<<(*itP)->str("            ")<<endl;
				}
				Dbg::dbg << "    transferred, modified="<<modified<<endl;
			}


			// XXX: Greg believes this plurality of
			// NodeState objects per DataflowNode is due
			// to FunctionCallExp, and may not even be
			// used there anymore, either

			// Look at the next NodeState
			i++; itS++;
		}
		//		ROSE_ASSERT(state);

		// =================== Populate the generated outgoing lattice to descendants (meetUpdate) ===================
		/*                      // if there has been a change in the dataflow state immediately below this node AND*/
		// If this is not the last node in the function
		if(*it != getUltimate(func)){

			if(analysisDebugLevel>=1){
				Dbg::dbg << " ==================================  "<<endl;
				Dbg::dbg << " Propagating/Merging the outgoing  Lattice to all descendant nodes ... "<<endl;
			}
			// iterate over all descendants
			vector<DataflowNode> descendants = getDescendants(n);
			if(analysisDebugLevel>=1) {
				Dbg::dbg << "    Descendants ("<<descendants.size()<<"):"<<endl;
				Dbg::dbg << "    ~~~~~~~~~~~~"<<endl;
			}

			for(vector<DataflowNode>::iterator di = descendants.begin(); di != descendants.end(); di++){

				// The CFG node corresponding to the current descendant of n
				DataflowNode nextNode = *di;
				SgNode *nextSgNode = nextNode.getNode();
				ROSE_ASSERT  (nextSgNode != NULL);
				if(analysisDebugLevel>=1)
					Dbg::dbg << "    Descendant: "<<nextSgNode<<"["<<nextSgNode->class_name()<<" | "<<Dbg::escape(nextSgNode->unparseToString())<<"]"<<endl;

				NodeState* nextState = NodeState::getNodeState(nextNode, 0);
				ROSE_ASSERT(nextSgNode && nextState);

				// Propagate the Lattices below this node to its descendant
				modified = propagateStateToNextNode(getLatticePost(state), n, numStates-1, getLatticeAnte(nextState), nextNode);

				// If the next node's state gets modified as a result of the propagation, 
				// add the node to the processing queue.
				if(modified)
					it.add(nextNode);
			}
		}

		if(analysisDebugLevel>=1) Dbg::exitFunc(nodeNameStr.str());        

	}

	bool modified = !NodeState::eqLattices(getLatticeAnte(*(NodeState::getNodeStates(getUltimate(func)).begin())),
			getLatticePost(fState));


	//	if(analysisDebugLevel>=1) Dbg::exitFunc(funcNameStr.str());

	return modified;
}

void PointerAliasAnalysis::runAnalysis(){
	std::vector<SgInitializedName *>globalVars = getGlobalVars(project);
	FunctionSet funcs = getDefinedFunctions(project);
	for(auto &func: funcs){
		FunctionState* fState = FunctionState::getFuncState(Function(func));
		printf("running\n");
		std::set<Function> calleesUpdated;

		doAnalysis(fState->func, &(fState->state), true, calleesUpdated);
		std::vector<FunctionDataflowInfo> retInfo;
		Rose_STL_Container<SgNode *> returnStmts = NodeQuery::querySubTree(func, V_SgReturnStmt);
		for(Rose_STL_Container<SgNode*>::const_iterator i = returnStmts.begin(); i != returnStmts.end(); ++i) {
			SgReturnStmt *returnStmt = isSgReturnStmt(*i);
			FunctionDataflowInfo info;
			info.returnStmt = returnStmt;
			NodeState *ns = getNodeStateForNode(returnStmt, filter);
			auto res = ns->getLatticeBelow(this);
			info.lattice = dynamic_cast<FiniteVarsExprsProductLattice *>(*(res.begin()));								
			retInfo.push_back(info);
		}
		functionRetInfo[func] = retInfo;		
		printf("done running\n");
	}
}
