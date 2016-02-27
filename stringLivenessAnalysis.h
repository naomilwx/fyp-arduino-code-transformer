#ifndef _STRINGLIVENESSANALYSIS_H_
#define _STRINGLIVENESSANALYSIS_H_
#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "liveStringsLattice.h"

class StringLivenessColouring : public IntraFWDataflow {
protected:
	StatementLiteralMap* slMap;
public:
	StringLivenessColouring(StatementLiteralMap *map){
		this->slMap = map;
	}
	void genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);
	bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
	boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
	LiveStringsFlowLattice::FlowVal getFlowValue(SgNode *n, const std::string& str);
	bool isBeforeStringLiteral(SgNode *n, const std::string& str);
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
	bool finish();
};

class StringLivenessAnalysis: public IntraBWDataflow {
protected:
	StringValPropagation *valMappings;
	StringLivenessColouring *livenessColouring;
//	LiteralMap *strLiteralInfoMap;
public:
	StringLivenessAnalysis(StringValPropagation *mappings, StringLivenessColouring *colouring){
		this->valMappings = mappings;
		this->livenessColouring = colouring;
	}
	void genInitState(const Function& func, const DataflowNode &n, const NodeState &state, std::vector<Lattice*>& initLattices, std::vector<NodeFact*>& initFacts);
	bool transfer(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);
	boost::shared_ptr<IntraDFTransferVisitor> getTransferVisitor(const Function& func, const DataflowNode& n, NodeState& state, const std::vector<Lattice*>& dfInfo);

	LiveStringsLattice *getLiveStrings(SgNode *node) const;
};

class StringLivenessAnalysisTransfer: public IntraDFTransferVisitor {
protected:
	StringValPropagation *valMappings;
	StringLivenessColouring *livenessColouring;

	LiveStringsLattice *liveStringsLat;
	StringSet usedStrings;

public:
	StringLivenessAnalysisTransfer(const Function &func, const DataflowNode &n, NodeState &s, const std::vector<Lattice *>& d, StringValPropagation* mappings, StringLivenessColouring* colouring): IntraDFTransferVisitor(func, n, s, d), liveStringsLat(dynamic_cast<LiveStringsLattice *>(*(dfInfo.begin()))), usedStrings(){
		this->liveStringsLat->initialize();
		this->valMappings = mappings;
		this->livenessColouring = colouring;
	}
	LiveStringsLattice *getLiveStrings() const;
//	void visit(SgStatement *stmt);
	void visit(SgVarRefExp *ref);
	void visit(SgStringVal *val);
	bool finish();
};
#endif
