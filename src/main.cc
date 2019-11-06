/*
 *
 *  Copyright 2013 Mario Alviano, Carmine Dodaro, and Francesco Ricca.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include <cstdlib>
#include <csignal>
#include <iostream>
#include <set>
#include <map>
#include <tuple>
#include <boost/process.hpp>
#include "WaspFacade.h"
#include "util/WaspOptions.h"
#include "Reconfigurator.h"
using namespace std;

using namespace boost::process;

int EXIT_CODE = 0;

WaspFacade* waspFacadePointer = NULL;

void my_handler( int )
{
    cerr << "Killed: Bye!" << endl;
    if( EXIT_CODE == 0 )
        EXIT_CODE = 1;
    waspFacadePointer->onKill();
    delete waspFacadePointer;
    Statistics::clean();
    exit( EXIT_CODE );
}

typedef std::tuple<std::string, int> signature;

signature toSignature(std::string line) {
    size_t slash = line.find_first_of("/");

    std::string name = line.substr(10, slash - 10);
    std::string arity_str = line.substr(slash + 1, 1);
    int arity = atoi(arity_str.c_str());

    return std::make_tuple(name, arity);
}

std::set<signature> getInstances(istream &input, ofstream &file) 
{
    std::string line;
    std::set<signature> sigs;

    while (input && std::getline(input, line)) {
        if (line.substr(0, 9) == "#instance") {
            sigs.insert(toSignature(line));
        } else {
            file << line << std::endl;
        }
    }

    return sigs;
}

bool matchSignature(std::string atom, set<signature> sigs) {
    size_t first_parens = atom.find_first_of("(");
    std::string name = atom.substr(0, first_parens);
    std::string params = atom.substr(first_parens + 1);

    int commas = 0;

    for (char c : params) {
        if(c == ',') { commas++; }
    }

    signature sig = std::make_tuple(name, commas + 1);
    return sigs.count(sig) == 1;
}

int main( int argc, char** argv )
{
    wasp::Options::parse( argc, argv );
    waspFacadePointer = new WaspFacade();
    WaspFacade& waspFacade = *waspFacadePointer;
    wasp::Options::setOptions( waspFacade );        
    
    signal( SIGINT, my_handler );
    signal( SIGTERM, my_handler );
    signal( SIGXCPU, my_handler );

    // Preprocess input encoding
    ipstream ground_stream;
    ofstream ostrm("/tmp/encoding.lp", std::ios::out);

    std::set<signature> sigs = getInstances(std::cin, ostrm);

    child c("gringo --output smodels /tmp/encoding.lp", std_out > ground_stream);
    waspFacade.readInput(ground_stream);
    c.wait();

    // Produce map of instance variables
    std::map<std::string, Var> instanceVariables;
    for (unsigned int i = 0; i < waspFacade.numberOfVariables(); i++) {
        if(!VariableNames::isHidden(i)) {
            std::string name = VariableNames::getName(i);
            if(matchSignature(name, sigs)) {
                instanceVariables.insert({name, i});
            }
        }
    }
    
    // Create and run Reconfigurator
    Reconfigurator reconf = Reconfigurator(waspFacade, instanceVariables);
    reconf.solve();
    
    waspFacade.onFinish();
    delete waspFacadePointer;
    Statistics::clean();
    return EXIT_CODE;
}
