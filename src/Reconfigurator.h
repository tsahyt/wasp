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
        Reconfigurator(WaspFacade& w, map<string, Var> v);
        ~Reconfigurator();
        void onLearningClause(Clause* clause) { clauses.insert(clause); }
        void onDeletingClause(Clause* clause) { clauses.erase(clause); }
        void solve();

    private:
        WaspFacade& waspFacade;
        unordered_set<Clause*> clauses;
        map<string, Var> instanceVariables;

        void processAssumptions(string line, map<Var,bool> assumptions, vector<Literal> assumptionsVec);
};
#endif /* end of include guard: RECONFIGURATOR_H */
