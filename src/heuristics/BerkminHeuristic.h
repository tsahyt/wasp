/*
 *
 *  Copyright 2013 Mario Alviano, Carmine Dodaro, Wolfgang Faber, Nicola Leone, Francesco Ricca, and Marco Sirianni.
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

/* 
 * File:   BerkminHeuristic.h
 * Author: Carmine Dodaro
 *
 * Created on 02 September 2013, 16.36
 */

#ifndef BERKMINHEURISTIC_H
#define	BERKMINHEURISTIC_H

#include "DecisionHeuristic.h"
#include "UndefinedCollector.h"

#include "../Trace.h"
#include "../Variable.h"

#include <cassert>
using namespace std;

class BerkminHeuristic : public DecisionHeuristic, public UndefinedCollector
{
    public:
        inline BerkminHeuristic( unsigned int numberOfLearnedClausesToConsider = static_cast< unsigned >( -1 ) );
        virtual ~BerkminHeuristic();
        
        virtual Literal makeAChoice( Solver& solver );
        virtual void onLearning( Solver& solver );
        virtual void onRestart( Solver& solver );
        
        virtual void onNewVariable( Variable& variable );
        
        virtual void collectUndefined( Variable* );
        
        virtual void onLiteralInvolvedInConflict( Literal literal );

    private:
        struct VariableCounters
        {
            public:
                VariableCounters() { counter[ 0 ] = counter[ 1 ] = globalCounter[ 0 ] = globalCounter[ 1 ] = 0; }
                
                unsigned getPositiveCounter() const { return counter[ 0 ]; }
                unsigned getNegativeCounter() const { return counter[ 1 ]; }
                unsigned getTotalCounter() const { return counter[ 0 ] + counter[ 1 ]; }
                
                /**
                 * Position 0 of this vector contains the heuristic counter of the positive literal associated with this variable.
                 * Position 1 of this vector contains the heuristic counter of the negative literal associated with this variable.
                 */
                // FIXME: introduce macros to refer to positive and negative positions
                unsigned counter[ 2 ];
                unsigned globalCounter[ 2 ];
        };
    
        inline void pickLiteralUsingActivity( Solver& solver );
        inline void pickLiteralFromTopMostUnsatisfiedLearnedClause( Solver& solver );        
        
        inline void choosePolarityTopMostUndefinedClause();
        inline void choosePolarityHigherGlobalCounter( Solver& solver );
        inline void choosePolarityMostOccurrences();
        inline void resetCounters();

        vector< VariableCounters > variableCounters;

        Variable* chosenVariableByOccurrences;
        unsigned int maxOccurrences;
        unsigned maxCounter;

        Variable* chosenVariable;
        bool chosenPolarity;
    
        unsigned int numberOfLearnedClausesToConsider;

        unsigned int estimatePropagation( Literal literal, Solver& solver );
        unsigned int numberOfConflicts;

        inline void updateMaxCounter( Variable* );
        inline void updateMaxOccurrences( Variable* );
};

BerkminHeuristic::BerkminHeuristic(
    unsigned int max ) : chosenVariableByOccurrences( NULL ), maxOccurrences( 0 ), maxCounter( 0 ), chosenVariable( NULL ), chosenPolarity( true ), numberOfLearnedClausesToConsider( max ), numberOfConflicts( 0 )
{
    // variable 0 is not used
    variableCounters.push_back( VariableCounters() );
}

void
BerkminHeuristic::resetCounters()
{
    maxCounter = maxOccurrences = 0;
    chosenVariableByOccurrences = NULL;
}

#endif	/* BERKMINHEURISTIC_H */

