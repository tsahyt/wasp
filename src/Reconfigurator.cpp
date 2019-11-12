#include "Reconfigurator.h"
#include "WaspFacade.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

Reconfigurator::Reconfigurator(WaspFacade& w, map<string, Var> v) : 
    waspFacade(w), instanceVariables(v) 
{
    waspFacade.attachClauseListener(this);
    mkfifo("reconfigurate", 0777);
};

Reconfigurator::~Reconfigurator() {
    unlink("reconfigurate");
}

void Reconfigurator::solve()
{
    ifstream istrm("reconfigurate", ios::in);
    string line;
    map<Var, bool> assumptions;

    for (auto var : instanceVariables) {
        assumptions[var.second] = false;
    }

    while(istrm && getline(istrm, line)) {
        vector<Literal> assumptionsVec;
        vector<Literal> conflict;

        processAssumptions(line, assumptions, assumptionsVec);

        unsigned int result = waspFacade.solve(assumptionsVec, conflict);
        if(result == COHERENT) {
            cout << "Coherent under assumptions" << endl;
        }
        else {
            assert(result == INCOHERENT);
            cout << "Incoherent under assumptions" << endl;
        }

        cout << "Learned clauses: " << clauses.size() << endl;
        for(auto it = clauses.begin(); it != clauses.end(); ++it) {
            Clause* clausePointer = *it;
            Clause& clause = *clausePointer;
            cout << "Processing clause:" << endl;
            for(unsigned int i = 0; i < clause.size(); i++) {
                cout << "\t Lit in position " << i << " - id: " << clause[i].getId();
                if(!VariableNames::isHidden(clause[i].getVariable()))
                    cout << " - name: " << VariableNames::getName(clause[i].getVariable()) << endl;
            }
            waspFacade.freeze(clausePointer); //Solver is restarted from level 0. After this it is not possible to retrieve the answer set (you have to call solve again)
        }
    }
}

void Reconfigurator::processAssumptions(string line, map<Var,bool> assumptions, vector<Literal> assumptionsVec) {
    cout << "Input: " << line << endl;
    istringstream iss(line);

    string word;
    while(iss && getline(iss, word, ' ')) {
        string atom = word.substr(0, word.size() - 2);
        Var var = instanceVariables.find(atom)->second;

        if(word[word.size() - 1] == '+') {
            assumptions[var] = true;
        }
        else if (word[word.size() - 1] == '-') {
            assumptions[var] = false;
        }
    }

    for (auto a : assumptions) {
        Literal l = Literal::createLiteralFromInt(a.second ? a.first : -a.first);
        assumptionsVec.push_back(l);
    }
}
