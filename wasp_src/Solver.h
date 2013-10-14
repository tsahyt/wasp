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
using namespace std;

#include "Clause.h"
#include "LearnedClause.h"
#include "Variable.h"
#include "Literal.h"
#include "WaspRule.h"
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
#include "outputBuilders/WaspOutputBuilder.h"
#include "heuristics/FirstUndefinedHeuristic.h"
#include "learning/RestartsBasedDeletionStrategy.h"
#include "learning/GeometricRestartsStrategy.h"
#include "stl/Vector.h"

class Solver
{
    public:
        inline Solver();
        ~Solver();
        
        inline void init();
        virtual bool preprocessing();
        virtual bool solve();
        inline void propagate( Literal literalToPropagate );
        
        void addVariable( const string& name );
        void addVariable();
        
//        AuxLiteral* addAuxVariable();
//        inline bool existsAuxLiteral( unsigned int id ) const;
//        inline AuxLiteral* getAuxLiteral( unsigned int id );
        
        inline void addClause( Clause* clause );
        inline void addLearnedClause( LearnedClause* learnedClause );
        bool addClauseFromModelAndRestart();
        
        Literal getLiteral( int lit );
        inline void addTrueLiteral( int lit );
        
        inline Literal getNextLiteralToPropagate();
        inline bool hasNextLiteralToPropagate() const;        
        
        inline const Variable* getNextAssignedVariable();
        inline bool hasNextAssignedVariable() const;
        inline void startIterationOnAssignedVariable();

        inline unsigned int getCurrentDecisionLevel();
        inline void incrementCurrentDecisionLevel();
        
        void onLiteralAssigned( Literal literal, Clause* implicant );
        
        inline bool propagateLiteralAsDeterministicConsequence( Literal literal );
        
        void decreaseLearnedClausesActivity();
        void onDeletingLearnedClausesThresholdBased();
        void onDeletingLearnedClausesAvgBased();
        inline void onLearningClause( Literal literalToPropagate, LearnedClause* learnedClause, unsigned int backjumpingLevel );
        inline void onLearningUnaryClause( Literal literalToPropagate, LearnedClause* learnedClause );        
        inline void onRestarting();        
        
        inline unsigned int numberOfClauses();
        inline unsigned int numberOfLearnedClauses();        
        inline unsigned int numberOfAssignedLiterals();
        inline unsigned int numberOfVariables();
        inline unsigned int numberOfAuxVariables();
        
        inline const UnorderedSet< Variable* >& getUndefinedVariables();
        inline const Vector< LearnedClause* >& getLearnedClauses();
        
        inline void setAChoice( Literal choice );        
        
        inline void analyzeConflict();
        inline void clearConflictStatus();
        inline void chooseLiteral();
        inline bool conflictDetected();
        inline void foundIncoherence();
        inline bool hasUndefinedLiterals();
        inline void printAnswerSet();        
        
        void unroll( unsigned int level );
        inline void unrollOne();
        inline void unrollLastVariable();
        
        void printProgram()
        {
            for( unsigned int i = 0; i < clauses.size(); ++i )
            {
                cout << *( clauses[ i ] ) << endl;
            }
        }
        
    private:
        Solver( const Solver& )
        {
            assert( "The copy constructor has been disabled." && 0 );
        }
                
        void addVariableInternal( Variable* variable );
        inline void deleteLearnedClause( LearnedClause* learnedClause, unsigned int position );//List< LearnedClause* >::iterator iterator );
        
        vector< Literal > trueLiterals;
        vector< Literal > literalsToPropagate;

        unsigned int currentDecisionLevel;        
        
        vector< Variable* > assignedVariables;
        int iteratorOnAssignedVariables;

        /* Data structures */
        vector< Variable* > variables;
        
        Vector< Clause* > clauses;
        Vector< LearnedClause* > learnedClauses;
        
        bool conflict;
        vector< unsigned int > unrollVector;
        
        Literal conflictLiteral;
        Clause* conflictClause;
        
        DecisionHeuristic* decisionHeuristic;
        HeuristicCounterFactoryForLiteral* heuristicCounterFactoryForLiteral;
        
        LearningStrategy* learningStrategy;
        DeletionStrategy* deletionStrategy;

        OutputBuilder* outputBuilder;
        
        UnorderedSet< Variable* > undefinedVariables;                
};

Solver::Solver() : currentDecisionLevel( 0 ), conflict( false ), conflictLiteral( NULL ), conflictClause( NULL )
{
    //Add a fake position.
    variables.push_back( NULL );
//    learningStrategy = new FirstUIPLearningStrategy( new SequenceBasedRestartsStrategy() );
    learningStrategy = new FirstUIPLearningStrategy( new SequenceBasedRestartsStrategy( 100000 ) );
//    deletionStrategy = new AggressiveDeletionStrategy();
    deletionStrategy = new RestartsBasedDeletionStrategy();
    
    heuristicCounterFactoryForLiteral = new BerkminCounterFactory();
    decisionHeuristic = new BerkminHeuristic();
    
    outputBuilder = new DimacsOutputBuilder();    
//    outputBuilder = new WaspOutputBuilder();    
}

void
Solver::init()
{
    cout << COMMENT_DIMACS << " " << WASP_STRING << endl;
}

void
Solver::propagate(
    Literal literalToPropagate )
{
    literalToPropagate.unitPropagation( *this );
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

void
Solver::addTrueLiteral(
    int lit )
{
    Literal literal = getLiteral( lit );
    trueLiterals.push_back( literal );
}

Literal
Solver::getNextLiteralToPropagate()
{
    assert( !literalsToPropagate.empty() );
    Literal tmp = literalsToPropagate.back();
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
    unrollVector.push_back( assignedVariables.size() );
    
    assert( currentDecisionLevel == unrollVector.size() );
}

void
Solver::unrollLastVariable()
{
    Variable* tmp = assignedVariables.back();
    assignedVariables.pop_back();
    tmp->setUndefined();        
    undefinedVariables.insert( tmp );
}

void
Solver::unrollOne()
{
    unroll( currentDecisionLevel - 1 );
}

void
Solver::onLearningClause( 
    Literal literalToPropagate, 
    LearnedClause* learnedClause, 
    unsigned int backjumpingLevel )
{
    assert( "Backjumping level is not valid." && backjumpingLevel < currentDecisionLevel );
    unroll( backjumpingLevel );    
    
    assert( "Each learned clause has to be an asserting clause." && literalToPropagate != NULL );
    assert( "Learned clause has not been calculated." && learnedClause != NULL );
    
    Clause* clause = static_cast< Clause* >( learnedClause );    
    onLiteralAssigned( literalToPropagate, clause );
    
    deletionStrategy->onLearning( *this, learnedClause );
}

void
Solver::onLearningUnaryClause(
    Literal literalToPropagate,
    LearnedClause* learnedClause )
{
    onRestarting();
    onLiteralAssigned( literalToPropagate, NULL );

    assert( "Learned clause has not been calculated." && learnedClause != NULL );    
    delete learnedClause;
}

void
Solver::onRestarting()
{
    deletionStrategy->onRestarting();
    decisionHeuristic->onRestarting( *this );
    unroll( 0 );
}

void
Solver::deleteLearnedClause( 
    LearnedClause* learnedClause,
    unsigned int position )//List< LearnedClause* >::iterator iterator )
{
    learnedClause->detachClause();

    delete learnedClause;
//    learnedClauses.erase( iterator );
    learnedClauses.erase( position );
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

const UnorderedSet< Variable* >&
Solver::getUndefinedVariables()
{
    return undefinedVariables;
}

const Vector< LearnedClause* >&
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
    return !undefinedVariables.empty();
}

void
Solver::printAnswerSet()
{
    outputBuilder->startModel();
    unsigned int size = assignedVariables.size();
    for( unsigned int i = 0; i < size; ++i )
    {
        const Variable* v = assignedVariables[ i ];
        if( !v->isHidden() )
            outputBuilder->printVariable( v );
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
    Literal choice = decisionHeuristic->makeAChoice( *this );    
    setAChoice( choice );
}

void
Solver::analyzeConflict()
{    
//    conflictLiteral.setOrderInThePropagation( numberOfAssignedLiterals() + 1 );
    learningStrategy->onConflict( conflictLiteral, conflictClause, *this );
    decisionHeuristic->onLearning( *this );
    clearConflictStatus();
}

void
Solver::clearConflictStatus()
{
    conflict = false;
    conflictLiteral = NULL;
    conflictClause = NULL;
}

unsigned int
Solver::numberOfAssignedLiterals()
{
    return assignedVariables.size();
}

unsigned int
Solver::numberOfVariables()
{
    return variables.size();
}

void
Solver::setAChoice(
    Literal choice )
{
    assert( choice != NULL );
    incrementCurrentDecisionLevel();
    assert( choice.isUndefined() );
    onLiteralAssigned( choice, NULL );
}

const Variable*
Solver::getNextAssignedVariable()
{
    const Variable* tmp = assignedVariables[ iteratorOnAssignedVariables-- ];    
    return tmp;
}

bool
Solver::hasNextAssignedVariable() const
{
    return iteratorOnAssignedVariables >= 0;
}

void
Solver::startIterationOnAssignedVariable()
{
    iteratorOnAssignedVariables = assignedVariables.size() - 1;
}

bool
Solver::propagateLiteralAsDeterministicConsequence(
    Literal literal )
{
    onLiteralAssigned( literal, NULL );
    if( conflictDetected() )
    {
        return false;
    }

    while( hasNextLiteralToPropagate() )
    {
        Literal literalToPropagate = getNextLiteralToPropagate();
//            literalToPropagate.setOrderInThePropagation( numberOfAssignedLiterals() );
        propagate( literalToPropagate );

        if( conflictDetected() )
        {
            return false;
        }
    }    
    assert( !conflictDetected() );

    return true;
}

//bool
//Solver::existsAuxLiteral(
//    unsigned int id ) const
//{
//    return( id < auxLiterals.size() );
//}
//
//AuxLiteral*
//Solver::getAuxLiteral(
//    unsigned int id )
//{
//    assert( existsAuxLiteral( id ) );
//    return auxLiterals[ id ];
//}

//unsigned int
//Solver::numberOfAuxVariables()
//{
//    return auxLiterals.size();
//}

#endif	/* SOLVER_H */

