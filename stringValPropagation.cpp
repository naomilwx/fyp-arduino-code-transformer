#include "boost/mem_fn.hpp"
#include <string>

#include "stringValPropagation.h"
#include "analysisCommon.h"
#include "ctUtils.h"
using std::auto_ptr;

using namespace FunctionAnalysisHelper;

// **********************************************************************
//                     PointerAliasAnalysisTransfer
// **********************************************************************


int PointerAliasAnalysisDebugLevel = 1;


PointerAliasAnalysisTransfer::PointerAliasAnalysisTransfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo, LiteralMap *map, PointerAliasAnalysis* analysis)
	: VariableStateTransfer<PointerAliasLattice>(func, n, state, dfInfo, PointerAliasAnalysisDebugLevel){
		this->literalMap = map;
		this->analysis = analysis;
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

	bool changed = false;
	//	getReturnAliasForFunctionCall
	if(isSgFunctionCallExp(rhs)) {
		std::vector<aliasDerefCount> rhsRefs = getReturnAliasForFunctionCall(isSgFunctionCallExp(rhs));
		for(auto &ref: rhsRefs) {
			resLat->setAliasRelation(make_pair(leftARNode, ref)) || changed;
		}
	}else {
		processRHS(rhs,rightARNode);
		//Establish the per CFG-node alias relations
		if((leftARNode.var !=NULL) && (rightARNode.var !=NULL))
			changed = resLat->setAliasRelation(make_pair(leftARNode,rightARNode)) || changed;
	}

	set< std::pair<aliasDerefCount, aliasDerefCount> > aliasRels = resLat->getAliasRelations();

	//Update the aliasedVariables(Compact Representation Graph)
	if(isSgPntrArrRefExp(lhs)){
		updateAliases(aliasRels,0);
	} else { 
		updateAliases(aliasRels,1);
	}

	//Update state of variable
	if(changed || isSgPntrArrRefExp(lhs)){
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
				} else {
					if(lhsLat->getAliasedVariables().size() > 1) {
						lhsLat->setState(PointerAliasLattice::REASSIGNED_UNKNOWN);
					} else if(lhsLat->getState() != PointerAliasLattice::REASSIGNED_UNKNOWN) {
						lhsLat->setState(PointerAliasLattice::REASSIGNED);
					}
				}
			}

		}
	}
}

std::vector<aliasDerefCount> PointerAliasAnalysisTransfer::getReturnAliasForFunctionCall(SgFunctionCallExp *fcall){
	Function callee(fcall);
	FunctionState* fState = FunctionState::getDefinedFuncState(callee);
	DFStateAtReturns* dfsar = dynamic_cast<DFStateAtReturns*>(fState->state.getFact(analysis, 0));

	std::vector<aliasDerefCount> refs;

	if(dfsar) {
		std::vector<Lattice*>& latsRetVal = dfsar->getLatsRetVal();
		Lattice *lat = NULL;
		for(auto &retLat: latsRetVal) {
			if(lat==NULL){
				lat = retLat->copy();
			} else {
				lat->meetUpdate(retLat);
			}
		}
		if(lat == NULL){
			return refs;
		}
		varID key("$");
		PointerAliasLattice *retAliasLat = dynamic_cast<PointerAliasLattice *>((dynamic_cast<VarsExprsProductLattice*>(lat))->getVarLattice(key));

		if(retAliasLat == NULL) {
			return refs;
		}
		SgExpressionPtrList params = fcall->get_args()->get_expressions();
		for(auto &alias: retAliasLat->getAliasedVariables()){
			std::string name = alias.str();
			printf("return val %s\n", name.c_str());
			int index = getFunctionParamNumberFromTag(name);
			aliasDerefCount paramNode;
			if(index >= 0) {
				processRHS(params[index], paramNode);
				refs.push_back(paramNode);

			} else if(isStringLiteralPlaceholder(name)) {
				paramNode.vID = alias;
				paramNode.derefLevel = -1;
				paramNode.var = isSgVariableSymbol(alias.components.at(0)->get_symbol_from_symbol_table());
				refs.push_back(paramNode);
			}
		}
	}
	return refs;
}



//Transfer function for Function Call Expressions. 
//Gets the lattice of the function call expression and updates lattice with alias information
void PointerAliasAnalysisTransfer::visit(SgFunctionCallExp *sgn) {
	//TODO: handle arguments modified by function
	propagateFunctionCallEffect(sgn);
	PointerAliasLattice *resLat = getLattice(sgn);
	updateAliases(resLat->getAliasRelations(),1);
}

void PointerAliasAnalysisTransfer::approximateFunctionCallEffect(SgFunctionCallExp *fcall){
	SgExpression *funcRef = getFunctionRef(fcall);
	SgExpressionPtrList params = fcall->get_args()->get_expressions();
	if(funcRef != NULL) {
		SgFunctionType *funcType = dynamic_cast<SgFunctionType *>(funcRef->get_type());
		SgTypePtrList fArgs = funcType->get_arguments();
		int argIdx = 0;
		for(auto &fArg : fArgs) {
			if(SageInterface::isPointerToNonConstType(fArg) || SageInterface::isReferenceType(fArg)) {
				PointerAliasLattice *lat = getLattice(params[argIdx]);
				lat->setState(PointerAliasLattice::MODIFIED);
			}
			argIdx++;
		}
	}
}

void PointerAliasAnalysisTransfer::propagateFunctionCallEffect(SgFunctionCallExp *fcall){
	Function callee(fcall);

	if(callee.get_definition()){

		FunctionState* funcS = FunctionState::getDefinedFuncState(callee);
		const vector<Lattice*>* funcLatticesAfter = &(funcS->state.getLatticeBelow(analysis));
		vector<Lattice*>::const_iterator itCalleeAfter, itCallerAfter;


		for(itCallerAfter = dfInfo.begin(), itCalleeAfter = funcLatticesAfter->begin();
				itCallerAfter!=dfInfo.end() && itCalleeAfter!=funcLatticesAfter->end();
				itCallerAfter++, itCalleeAfter++){
			ctVarsExprsProductLattice* callerL =dynamic_cast<ctVarsExprsProductLattice*>(*itCallerAfter);
			ctVarsExprsProductLattice* calleeL = dynamic_cast<ctVarsExprsProductLattice*>(*itCalleeAfter);

			//			// Create a copy of the current lattice, remapped for the callee function's variables
			Lattice* remappedL = calleeL->copy();

			map<varID, varID> paramArgByRefMap = getPlaceholderToArgMap(fcall);

			remappedL->remapVars(paramArgByRefMap, func);
			callerL->incorporateVars(remappedL, [](Lattice *lat){
					PointerAliasLattice* l = dynamic_cast<PointerAliasLattice *>(lat);
					if(l) {
					return (l->getState() <= PointerAliasLattice::INITIALIZED);
					}
					return true;
					});

		}
	}else {
		approximateFunctionCallEffect(fcall);
	}
}

std::map<varID,varID> PointerAliasAnalysisTransfer::getPlaceholderToArgMap(SgFunctionCallExp *fcall){
	SgExpressionPtrList args = fcall->get_args()->get_expressions();
	int idx = 0;
	std::map<varID,varID> res;

	for(auto& arg: args){
		std::string placeholder = getPlaceholderNameForArgNum(idx);
		//		if(isVarExpr(arg)){
		res[varID(placeholder)] = SgExpr2Var(arg);
		//		}
		idx++;
	}
	return res;
}

void PointerAliasAnalysisTransfer::visit(SgFunctionDefinition *fdef) {
	SgFunctionParameterList *params = isSgFunctionDeclaration(fdef->get_parent())->get_parameterList();
	SgInitializedNamePtrList args = params->get_args();
	int index = 0;

	SgScopeStatement *scope = fdef->get_scope();

	for(SgInitializedName *arg: args) {
		aliasDerefCount left, right;

		processLHS(arg, left);

		PointerAliasLattice* lhsLat =  getLattice(left.vID);

		if(lhsLat){
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
	if(derefLevel==0) {
		result.insert(var);
	} else if(lat){
		set<varID> outS = lat->getAliasedVariables();
		for(set<varID>::iterator outVar = outS.begin(); outVar != outS.end(); outVar++) {
			computeAliases(getLattice(*outVar),*outVar,derefLevel-1,result);
		}
	}
}

void PointerAliasAnalysisTransfer::processParam(int index, SgScopeStatement *scope, SgInitializedName *param, struct aliasDerefCount &arNode){
	SgType *type = param->get_type();
	std::string name = getPlaceholderNameForArgNum(index);
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
				if(SageInterface::isReferenceType(var_exp->get_type())) {
					derefLevel++;
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
	PointerAliasAnalysisTransfer::processRHS(node, arNode, this->literalMap);
}

void PointerAliasAnalysisTransfer::processRHS(SgNode *node, struct aliasDerefCount &arNode, LiteralMap *literalMap) {
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
				processRHS(ptr_exp->get_operand(), arNode, literalMap);
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
				processRHS(add_exp->get_operand(), arNode, literalMap);
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
				processRHS(bin_exp->get_rhs_operand(), arNode, literalMap);
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
					processRHS(rhs, arNode, literalMap);
					return;
				}
			}
			break;

		case V_SgCastExp:
			{
				SgCastExp *cast_exp = isSgCastExp(node);
				processRHS(cast_exp->get_operand(), arNode, literalMap);
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
	auto res = new ctVarsExprsProductLattice((Lattice*) new PointerAliasLattice(), emptyM, (Lattice*)NULL,NULL, n, state, globalVarsLattice);
	if(func.get_definition()) {
		int idx = 0;
		for(auto&param: func.get_declaration()->get_args()) {
			Lattice *lat = res->addSlotForVariable(varID(getPlaceholderNameForArgNum(idx)));
			dynamic_cast<PointerAliasLattice *>(lat)->setState(PointerAliasLattice::INITIALIZED);
			idx++;
		}
	}
	initLattices.push_back(res);
	//	for(auto &item: globalVarsLattice)
	//		printf("%s: %s", item.first.str().c_str(), item.second->str(" ").c_str());
	//printf("init node %s\n", n.getNode()->class_name().c_str());
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
	return boost::shared_ptr<IntraDFTransferVisitor>(new PointerAliasAnalysisTransfer(func, n, state, dfInfo, literalMap, this));
}

PointerAliasLattice *PointerAliasAnalysis::getAliasLattice(NodeState *s, varID var){
	ctVarsExprsProductLattice *lat = dynamic_cast<ctVarsExprsProductLattice *>(*(s->getLatticeBelow(this).begin()));
	return dynamic_cast<PointerAliasLattice *>(lat->getVarLattice(var)); 
}

std::set<varID> PointerAliasAnalysis::getAliasesForVariableAtNode(SgNode *node, varID var) {
	NodeState *ns = getNodeStateForNode(node, filter);
	PointerAliasLattice *lat = getAliasLattice(ns, var);
	return lat->getAliasedVariables();
}


void PointerAliasAnalysis::setGlobalAliasRelationForLat(PointerAliasLattice *lat, aliasDerefCount& lhs, SgNode *rhsExp){
	aliasDerefCount rhs;
	PointerAliasAnalysisTransfer::processRHS(rhsExp, rhs, literalMap);
	if((lhs.var !=NULL) && (rhs.var !=NULL)){
		lat->setAliasRelation(make_pair(lhs,rhs));
		set<varID> result;
		computeGlobalAliases(lat, rhs.vID, rhs.derefLevel + 1, result);
		lat->setAliasedVariables(result);
	}
}

void PointerAliasAnalysis::computeGlobalAliases(PointerAliasLattice *lat, varID var, int derefLevel, set<varID> &result)
{
	if(derefLevel==0)
		result.insert(var);
	else
	{
		set<varID> outS = lat->getAliasedVariables();
		for(set<varID>::iterator outVar = outS.begin(); outVar != outS.end(); outVar++)
		{
			PointerAliasLattice *outLat = dynamic_cast<PointerAliasLattice *>(globalVarsLattice[*outVar]);
			if(outLat)
				computeGlobalAliases(outLat,*outVar,derefLevel-1,result);
		}
	}
}
void PointerAliasAnalysis::runGlobalVarAnalysis() {

	std::vector<SgInitializedName *> vars = getGlobalVars(project);
	for(auto &initName: vars) {
		aliasDerefCount lhs;
		PointerAliasAnalysisTransfer::processLHS(initName, lhs);

		PointerAliasLattice* lat = new PointerAliasLattice();
		SgInitializer* initializer = initName->get_initializer();
		if(initializer) {
			lat->setState(PointerAliasLattice::INITIALIZED);
			if(isSgAssignInitializer(initializer)) {
				SgExpression *rhsExp = isSgAssignInitializer(initializer)->get_operand();
				setGlobalAliasRelationForLat(lat, lhs, rhsExp);
			}else if(isSgAggregateInitializer(initializer)) {
				SgExprListExp *lst = isSgAggregateInitializer(initializer)->get_initializers();
				for(auto &exp: lst->get_expressions()) {
					setGlobalAliasRelationForLat(lat, lhs, exp);
				}
			}
		}

		globalVarsLattice[varID(initName)] = lat;
	}
}

void PointerAliasAnalysis::transferFunctionCall(const Function &func, const DataflowNode &n, NodeState *state) {
	vector<Lattice*> dfInfoBelow = state->getLatticeBelow(this);
	vector<Lattice*>* retState = NULL;
	ctOverallDataflowAnalyser *interAnalyser = dynamic_cast<ctOverallDataflowAnalyser*>(interAnalysis);
	if(interAnalyser) {
		interAnalyser->transfer(func, n, *state, dfInfoBelow, true, false);
	}
}

void PointerAliasAnalysis::runAnalysis() {
	runGlobalVarAnalysis();

	ctOverallDataflowAnalyser overallAnalyser(project, this);
	overallAnalyser.runAnalysis();
}

bool PointerAliasAnalysis::isUnmodifiedStringOrCharArray(SgFunctionDeclaration *func, SgNode *exp) {
	PointerAliasLattice *lat = getReturnStateAliasLattice(func, exp);
		if(lat) {
			return lat->getState() != PointerAliasLattice::MODIFIED;
		}
		return false;
}

bool PointerAliasAnalysis::isMultiAssignmentPointer(SgFunctionDeclaration *func, SgNode *exp) {
	PointerAliasLattice *lat = getReturnStateAliasLattice(func, exp);
	if(lat) {
		return lat->getState() == PointerAliasLattice::REASSIGNED_UNKNOWN;
	}
	return false;
}

PointerAliasLattice *PointerAliasAnalysis::getReturnStateAliasLattice(SgFunctionDeclaration *func, SgNode *exp) {
	if(varID::isValidVarExp(exp)) {
		ctVarsExprsProductLattice *retLat = getReturnStateLattice(func);
		if(retLat) {
			return dynamic_cast<PointerAliasLattice *>(retLat->getVarLattice(varID(exp)));
		}
	}
	return NULL;
}

ctVarsExprsProductLattice * PointerAliasAnalysis::getReturnStateLattice(SgFunctionDeclaration *func) {
	Function fdecl(func);
	FunctionState* fState = FunctionState::getDefinedFuncState(fdecl);
	DFStateAtReturns* dfsar = dynamic_cast<DFStateAtReturns*>(fState->state.getFact(this, 0));

	if(dfsar) {
			std::vector<Lattice*>& retLats = dfsar->getLatsAtFuncReturn();
			Lattice *lat = NULL;
			for(auto &retLat: retLats) {
				if(lat==NULL){
					lat = retLat->copy();
				} else {
					lat->meetUpdate(retLat);
				}
			}

			return dynamic_cast<ctVarsExprsProductLattice*>(lat);

		}
	return NULL;
}
