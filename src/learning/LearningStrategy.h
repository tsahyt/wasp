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
 * File:   LearningStrategy.h
 * Author: Carmine Dodaro
 *
 * Created on 27 July 2013, 9.51
 */

#ifndef LEARNINGSTRATEGY_H
#define	LEARNINGSTRATEGY_H

#include "restarts/RestartsStrategy.h"

#include <cassert>
#include <iostream>
#include <unordered_set>

using namespace std;

class Clause;
class LearnedClause;
class Literal;
class Solver;
class Variable;

class LearningStrategy
{
    public:
        inline LearningStrategy( RestartsStrategy* restartsStrategy );
        inline ~LearningStrategy();
        
        void onNavigatingLiteral( Literal );
        void onConflict( Literal conflictLiteral, Clause* conflictClause, Solver& solver );
        
    private:
        /**
         * This method cleans data structures.
         * It should be called in the end of each iteration.
         */
        inline void clearDataStructures();           
        
        /**
         * Add a literal in the new learned clause.
         * @param literal the literal to add.
         */
        inline void addLiteralInLearnedClause( Literal literal );
        
        /**
         * The literal added by this method is a literal which should be navigated.
         * @param literal the literal to navigate.
         */
        inline void addLiteralToNavigate( Literal literal );     
        
        /**
         * The decision level of the conflict.
         */
        unsigned int decisionLevel;
        
        /**
         * The new learned clause.
         */
        LearnedClause* learnedClause;
        
        /**
         * The strategy used for restarts.
         */
        RestartsStrategy* restartsStrategy;             

        /**
         * This method computes the next literal to navigate in the implication graph.
         * The most recent (in the order of derivation) literal should be processed before.          
         * 
         * @return the next literal to consider.
         */
        Literal getNextLiteralToNavigate( Solver& solver );
        
        /**
         * The variables of the current conflict level which have been visited.
         */
        unordered_set< const Variable* > visitedVariables;        
        
        unsigned int maxDecisionLevel;
        
        unsigned int maxPosition;
        
        unsigned int visitedVariablesCounter;
};

LearningStrategy::LearningStrategy(
    RestartsStrategy* strategy ): decisionLevel( 0 ), learnedClause( NULL ), maxDecisionLevel( 0 ), visitedVariablesCounter( 0 )
{
    assert( "The strategy for restarts must be initialized." && strategy != NULL );
    restartsStrategy = strategy;
}

LearningStrategy::~LearningStrategy()
{
    if( restartsStrategy )
        delete restartsStrategy;
}

void
LearningStrategy::clearDataStructures()
{
    learnedClause = NULL;    
    maxDecisionLevel = 0;
    visitedVariables.clear();
    visitedVariablesCounter = 0;
}

#endif	/* LEARNINGSTRATEGY_H */

