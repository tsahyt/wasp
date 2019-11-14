#include "Reconfigurator.h"
#include "WaspFacade.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/process.hpp>

using namespace boost::process;

Reconfigurator::Reconfigurator(WaspFacade& w, map<string, Var> v, map<string,Var> a, string r) :
    waspFacade(w), instanceVariables(v), allVariables(a), relaxedProgram(r)
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

    for (auto var : instanceVariables) {
        assumptions[var.second] = false;
    }

    while(istrm && getline(istrm, line)) {
        vector<Literal> conflict;

        vector<Literal> assumptions_vec = processAssumptions(line);
        unordered_set<Var> relaxed_answer_set = computeRelaxedAnswerSet();

        unsigned int result = waspFacade.solve(assumptions_vec, conflict);
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

vector<Literal> Reconfigurator::processAssumptions(string line) {
    vector<Literal> avec;
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
        avec.push_back(l);
    }

    return avec;
}

string Reconfigurator::relaxedAssumptions() {
    string prog;
    prog.append(relaxedProgram);

    for (auto a : assumptions) {
        if(!a.second) continue;
        string x = VariableNames::getName(a.first);
        prog.append(x + ".");
    }

    return prog;
}

unordered_set<Var> Reconfigurator::computeRelaxedAnswerSet() {
    string line;
    ipstream astream;
    ofstream program_file("/tmp/relaxed.lp", std::ios::out);
    unordered_set<Var> answer_set;

    program_file << relaxedAssumptions() << endl;
    child clingo("clingo --outf=1 /tmp/relaxed.lp", std_out > astream);

    stringstream answer_stream;
    while(astream && std::getline(astream, line)) {
        if(line[0] == '%') continue;
        answer_stream << line << endl;
    }

    string atom;
    while(getline(answer_stream, atom, ' ')) {
        auto v = allVariables[atom.substr(0, atom.size() - 1)];
        answer_set.insert(v);
    }

    clingo.wait();

    return answer_set;
}
