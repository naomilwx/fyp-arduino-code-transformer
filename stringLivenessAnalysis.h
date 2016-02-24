#include "stringValPropagation.h"
#include "stringLiteralAnalysis.h"
#include "liveStringsLattice.h"

class StringLivenessAnalysis: public IntraBWDataflow {
protected:
	StringValPropagation *valMappings;
	LiteralMap *strLiteralInfoMap;
};

class StringLivenessAnalysisTransfer: public IntraDFTransferVisitor {
protected:
	StringValPropagation *valMappings;
	LiveStringsLattice *liveStringsLat;
	std::set<std::string> usedStrings;

public:
	LiveStringsLattice *getLiveStrings() const;
};


class StringLivenessColouring : public IntraFWDataflow {

};
