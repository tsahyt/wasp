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
//    mkfifo("reconfigurate", 0777);
};

Reconfigurator::~Reconfigurator() {
 //   unlink("reconfigurate");
}

void Reconfigurator::solve()
{
//    ifstream istrm("reconfigurate", ios::in);
    stringstream istrm("preplace(1,1);+\npreplace(1,1);-\npreplace(1,1);+");
    string line;

    for (auto var : instanceVariables) {
        assumptions[var.second] = false;
    }

    while(istrm && getline(istrm, line)) {
        vector<Literal> conflict;
        int nClauses = clauses.size();

        // Process Assumptions and retrieve relaxed answer set
        vector<Literal> assumptions_vec = processAssumptions(line);
        unordered_set<Var> relaxed_answer_set = computeRelaxedAnswerSet();

        // Manage clauses
        for(auto it = clauses.begin(); it != clauses.end(); ++it) {
            Clause& clause = **it;
            bool contained = true;
            for(unsigned int i = 0; i < clause.size(); ++i) {
                Literal l = clause[i];
                if(l.isPositive()) {
                    contained &= relaxed_answer_set.count(l.getVariable()) == 1;
                } else {
                    contained &= relaxed_answer_set.count(l.getVariable()) == 0;
                }
            }
            if(contained) {
                waspFacade.thaw(&clause);
                cerr << "Unfreezing " << &clause << endl;
            } else {
                waspFacade.freeze(&clause);
                cerr << "Freezing " << &clause << endl;
            }
        }

        unsigned int result = waspFacade.solve(assumptions_vec, conflict);
        if(result == COHERENT) {
            cout << "Coherent under assumptions" << endl;
        }
        else {
            assert(result == INCOHERENT);
            cout << "Incoherent under assumptions" << endl;
        }

        cout << "Learned " << clauses.size() - nClauses << " clauses" << endl;
    }
}

vector<Literal> Reconfigurator::processAssumptions(string line) {
    vector<Literal> avec;
    cout << "Input: " << line << endl;
    istringstream iss(line);

    string word;
    while(iss && getline(iss, word, ' ')) {
        string atom = word.substr(0, word.size() - 2);
        auto search = instanceVariables.find(atom);

        if(search != instanceVariables.end()) {
            Var var = search->second;
            if(word[word.size() - 1] == '+') {
                assumptions[var] = true;
            }
            else if (word[word.size() - 1] == '-') {
                assumptions[var] = false;
            }
        } else {
            cerr << "WARNING: Ignoring unknown atom " << atom << endl;
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
