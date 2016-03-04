#ifndef _STRINGLIVENESSANALYSIS_H_
#define _STRINGLIVENESSANALYSIS_H_
#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "liveStringsLattice.h"

class StringLivenessColouring : public IntraFWDataflow {
private:
	bool ranAnalysis;
	SgProject *project;
protected:
	StatementLiteralMap* slMap;
public:
	StringLivenessColouring(SgProject *project, StatementLiteralMap *map): ranAnalysis(false){
		this->slMap = map;
		this->project = project;
	}
	bool hasRunAnalysis() {
		return ranAnalysis;
	}
	void genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);
	bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
	boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

	LiveStringsFlowLattice* getLatticeForNode(SgNode *n);
	LiveStringsFlowLattice* getLatticeForNodeState(const NodeState &s);

	LiveStringsFlowLattice::FlowVal getFlowValue(const NodeState &s, const std::string& str);

	bool isBeforeStringLiteral(const NodeState &s, const std::string& str);

	bool isBeforeStringVar(const NodeState &s, varID var);

	void runOverallAnalysis();
};

class StringLivenessColouringTransfer: public IntraDFTransferVisitor {
protected:
	StatementLiteralMap* slMap;
	LiveStringsFlowLattice *flowLattice;
	bool modified;
public:
	StringLivenessColouringTransfer(const Function &func, const DataflowNode &n, NodeState &s, const std::vector<Lattice *>& d, StatementLiteralMap *slMap): IntraDFTransferVisitor(func, n, s, d), flowLattice(dynamic_cast<LiveStringsFlowLattice *>(*(dfInfo.begin()))), modified(false){
		this->slMap = slMap;
	}
	void visit(SgStatement *n);
	void visit(SgInitializedName *n);
	bool finish();
};

class StringLivenessAnalysis: public IntraBWDataflow {
protected:
	SgProject *project;
	StringValPropagation *valMappings;
	StringLivenessColouring *livenessColouring;
private:
	bool destroyColouring;
	bool ranAnalysis;

public:
	StringLivenessAnalysis(SgProject *project, StringValPropagation *mappings, StringLivenessColouring *colouring): destroyColouring(false), ranAnalysis(false){
		this->project = project;
		this->valMappings = mappings;
		this->livenessColouring = colouring;
	}
	StringLivenessAnalysis(SgProject *project, StringValPropagation *mappings, StatementLiteralMap *slMap): destroyColouring(true), ranAnalysis(false){
		this->project = project;
		this->valMappings = mappings;
		this->livenessColouring = new StringLivenessColouring(project, slMap);
	}
	~StringLivenessAnalysis() {
		if(destroyColouring) {
			delete this->livenessColouring;
		}
	}

	SgProject *getProject() {
		return project;
	}

	StringLivenessColouring *getLivenessColouring(){
		return livenessColouring;
	}

	void genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);
	bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
	boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

	LiveStringsLattice *getLiveIn(SgStatement *node) const;
	LiveStringsLattice *getLiveOut(SgStatement *node) const;
	void runOverallAnalysis();
	void runAnnotation();
};

class StringLivenessAnalysisTransfer: public IntraDFTransferVisitor {
protected:
	StringValPropagation *valMappings;
	StringLivenessColouring *livenessColouring;

	LiveStringsLattice *liveStringsLat;
	StringSet usedStrings;
	std::set<varID> usedVars;
public:
	StringLivenessAnalysisTransfer(const Function &func, const DataflowNode &n, NodeState &s, const std::vector<Lattice *>& d, StringValPropagation* mappings, StringLivenessColouring* colouring): IntraDFTransferVisitor(func, n, s, d), liveStringsLat(dynamic_cast<LiveStringsLattice *>(*(dfInfo.begin()))), usedStrings(){
		this->liveStringsLat->initialize();
		this->valMappings = mappings;
		this->livenessColouring = colouring;
	}

	LiveStringsLattice *getLiveStrings() const;
	void visit(SgVarRefExp *ref);
	void visit(SgStringVal *val);
	bool finish();
};

#endif
