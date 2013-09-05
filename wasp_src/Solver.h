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
 * File:   Solver.h
 * Author: Carmine Dodaro
 *
 * Created on 21 July 2013, 17.36
 */

#ifndef SOLVER_H
#define	SOLVER_H

#include <cassert>
#include <vector>
#include <list>
using namespace std;

#include "Clause.h"
#include "LearnedClause.h"
#include "Literal.h"
#include "PositiveLiteral.h"
#include "stl/List.h"
#include "stl/UnorderedSet.h"
#include "learning/AggressiveDeletionStrategy.h"
#include "learning/DeletionStrategy.h"
#include "learning/FirstUIPLearningStrategy.h"
#include "learning/LearningStrategy.h"
#include "learning/SequenceBasedRestartsStrategy.h"
#include "heuristics/BerkminHeuristic.h"
#include "heuristics/DecisionHeuristic.h"
#include "heuristics/factories/HeuristicCounterFactoryForLiteral.h"
#include "heuristics/factories/BerkminCounterFactory.h"
#include "outputBuilders/OutputBuilder.h"
#include "outputBuilders/DimacsOutputBuilder.h"

class Variable;

class Solver
{
    public:
        inline Solver();
        ~Solver();
        
        virtual void init() = 0;
        virtual bool solve();
        virtual void propagate( Literal* literalToPropagate ) = 0;
        
        void addVariable( const string& name );
        void addVariable();
        
        inline void addClause( Clause* clause );
        inline void addLearnedClause( LearnedClause* learnedClause );        
        
        Literal* getLiteral( int lit );
        
        inline Literal* getNextLiteralToPropagate();
        inline bool hasNextLiteralToPropagate() const;        
        
        inline unsigned int getCurrentDecisionLevel();
        inline void incrementCurrentDecisionLevel();
        
        void onLiteralAssigned( Literal* literal, TruthValue truthValue, Clause* implicant );        
        
        void decreaseLearnedClausesActivity();
        void onDeletingLearnedClausesThresholdBased();
        void onDeletingLearnedClausesAvgBased();
        inline void onLearningClause( Literal* literalToPropagate, LearnedClause* learnedClause, unsigned int backjumpingLevel );
        inline void onLearningUnaryClause( Literal* literalToPropagate, LearnedClause* learnedClause );        
        inline void onRestarting();        
        
        inline unsigned int numberOfClauses();
        inline unsigned int numberOfLearnedClauses();        
        inline unsigned int numberOfAssignedLiterals();
        
        inline const UnorderedSet< PositiveLiteral* >& getUndefinedLiterals();
        inline const List< LearnedClause* >& getLearnedClauses();
        
        inline void setAChoice( Literal* choice );
        
        void unroll( unsigned int level );
        inline void unrollOne();
        
        void printProgram()
        {
            for( list< Clause* >::iterator it = clauses.begin(); it != clauses.end(); ++it )
            {
                cout << *( *it ) << endl;
            }
        }        
        
    private:
        Solver( const Solver& )
        {
            assert( "The copy constructor has been disabled." && 0 );
        }
                
        void addVariableInternal( PositiveLiteral* posLiteral );
        inline void deleteLearnedClause( LearnedClause* learnedClause, List< LearnedClause* >::iterator iterator );
        
        list< Literal* > literalsToPropagate;
        unsigned int currentDecisionLevel;
        
        List< PositiveLiteral* > assignedLiterals;
        
        /* Data structures */
        vector< PositiveLiteral* > positiveLiterals;
        List< Clause* > clauses;
        List< LearnedClause* > learnedClauses;
        
        bool conflict;
        vector< unsigned int > unrollVector;
        
        Literal* conflictLiteral;
        
        DecisionHeuristic* decisionHeuristic;
        HeuristicCounterFactoryForLiteral* heuristicCounterFactoryForLiteral;
        
        LearningStrategy* learningStrategy;
        DeletionStrategy* deletionStrategy;
        
        OutputBuilder* outputBuilder;
        
        UnorderedSet< PositiveLiteral* > undefinedLiterals;
        
    protected:
        inline void analyzeConflict();
        inline void chooseLiteral();
        inline bool conflictDetected();
        inline void foundIncoherence();
        inline bool hasUndefinedLiterals();
        inline void printAnswerSet();
};

Solver::Solver() : currentDecisionLevel( 0 ), conflict( false ), conflictLiteral( NULL )
{
    //Add a fake position.
    positiveLiterals.push_back( NULL );
    learningStrategy = new FirstUIPLearningStrategy( new SequenceBasedRestartsStrategy() );
    deletionStrategy = new AggressiveDeletionStrategy();
    
    heuristicCounterFactoryForLiteral = new BerkminCounterFactory();
    decisionHeuristic = new BerkminHeuristic();
    
    outputBuilder = new DimacsOutputBuilder();
}

void
Solver::addClause(
    Clause* clause )
{
    clauses.push_back( clause );
}

void
Solver::addLearnedClause( 
    LearnedClause* learnedClause )
{    
    learnedClauses.push_back( learnedClause );
}

Literal*
Solver::getNextLiteralToPropagate()
{
    assert( !literalsToPropagate.empty() );
    Literal* tmp = literalsToPropagate.back();
    literalsToPropagate.pop_back();
    return tmp;
}
        
bool
Solver::hasNextLiteralToPropagate() const
{
    return !literalsToPropagate.empty();
}

unsigned int
Solver::getCurrentDecisionLevel()
{
    return currentDecisionLevel;
}

void
Solver::incrementCurrentDecisionLevel()
{
    currentDecisionLevel++;
    unrollVector.push_back( assignedLiterals.size() );
    
    assert( currentDecisionLevel == unrollVector.size() );
}

void
Solver::unrollOne()
{
    unroll( currentDecisionLevel - 1 );
}

void
Solver::onLearningClause( 
    Literal* literalToPropagate, 
    LearnedClause* learnedClause, 
    unsigned int backjumpingLevel )
{
    assert( "Backjumping level is not valid." && backjumpingLevel < currentDecisionLevel );
    unroll( backjumpingLevel );    
    
    assert( "Each learned clause has to be an asserting clause." && literalToPropagate != NULL );
    assert( "Learned clause has not been calculated." && learnedClause != NULL );
    
    Clause* clause = static_cast< Clause* >( learnedClause );
    onLiteralAssigned( literalToPropagate, TRUE, clause );
    
    deletionStrategy->onLearning( *this, learnedClause );
}

void
Solver::onLearningUnaryClause(
    Literal* literalToPropagate,
    LearnedClause* learnedClause )
{
    onRestarting();
    onLiteralAssigned( literalToPropagate, TRUE, NULL );

    assert( "Learned clause has not been calculated." && learnedClause != NULL );    
    delete learnedClause;
}

void
Solver::onRestarting()
{
    deletionStrategy->onRestarting();
    unroll( 0 );
}

void
Solver::deleteLearnedClause( 
    LearnedClause* learnedClause,
    List< LearnedClause* >::iterator iterator )
{
    learnedClause->detachClause();
    delete learnedClause;
    learnedClauses.erase( iterator );
}

unsigned int
Solver::numberOfClauses()
{
    return clauses.size();
}

unsigned int
Solver::numberOfLearnedClauses()
{
    return learnedClauses.size();
}

const UnorderedSet< PositiveLiteral* >&
Solver::getUndefinedLiterals()
{
    return undefinedLiterals;
}

const List< LearnedClause* >&
Solver::getLearnedClauses()
{
    return learnedClauses;
}

bool
Solver::conflictDetected()
{
    return conflict;
}

bool
Solver::hasUndefinedLiterals()
{
    return !undefinedLiterals.empty();
}

void
Solver::printAnswerSet()
{
    outputBuilder->startModel();    
    for( List< PositiveLiteral* >::iterator it = assignedLiterals.begin(); it != assignedLiterals.end(); ++it )
    {
        outputBuilder->printLiteral( *it );
    }
    outputBuilder->endModel();
}

void
Solver::foundIncoherence()
{
    outputBuilder->onProgramIncoherent();
}

void
Solver::chooseLiteral()
{
    Literal* choice = decisionHeuristic->makeAChoice( *this );
    setAChoice( choice );
}

void
Solver::analyzeConflict()
{
    conflict = false;
    conflictLiteral->setOrderInThePropagation( numberOfAssignedLiterals() + 1 );
    learningStrategy->onConflict( conflictLiteral, *this );
    conflictLiteral = NULL;
}

unsigned int
Solver::numberOfAssignedLiterals()
{
    return assignedLiterals.size();
}

void
Solver::setAChoice( 
    Literal* choice )
{
    assert( choice != NULL );
    incrementCurrentDecisionLevel();
    assert( choice->isUndefined() );
    onLiteralAssigned( choice, TRUE, NULL );
    cout << "Choice " << *choice << endl;
}

#endif	/* SOLVER_H */
