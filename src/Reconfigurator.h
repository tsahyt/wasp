#ifndef RECONFIGURATOR_H
#define RECONFIGURATOR_H

#include "ClauseListener.h"
#include "WaspFacade.h"

#include <unordered_set>
#include <map>

using namespace std;

class Reconfigurator : public ClauseListener
{
    public:
        Reconfigurator(WaspFacade& w, map<string, Var> v, map<string, Var> a, string r);
        ~Reconfigurator();
        void onLearningClause(Clause* clause) { clauses.insert(clause); }
        void onDeletingClause(Clause* clause) { clauses.erase(clause); }
        void solve();

    private:
        WaspFacade& waspFacade;
        unordered_set<Clause*> clauses;
        map<string, Var> instanceVariables;
        map<string, Var> allVariables;

        string relaxedProgram;
        map<Var, bool> assumptions;

        vector<Literal> processAssumptions(string line);
        string relaxedAssumptions();
        unordered_set<Var> computeRelaxedAnswerSet();
};
#endif /* end of include guard: RECONFIGURATOR_H */
