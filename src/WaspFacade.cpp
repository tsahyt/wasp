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

#include "WaspFacade.h"

#include "learning/restarts/NoRestartsStrategy.h"
#include "learning/restarts/MinisatRestartsStrategy.h"
#include "learning/restarts/GeometricRestartsStrategy.h"
#include "learning/restarts/SequenceBasedRestartsStrategy.h"

#include "learning/deletion/AggressiveDeletionStrategy.h"
#include "learning/deletion/RestartsBasedDeletionStrategy.h"
#include "learning/deletion/MinisatDeletionStrategy.h"

#include "outputBuilders/WaspOutputBuilder.h"
#include "outputBuilders/SilentOutputBuilder.h"
#include "outputBuilders/ThirdCompetitionOutputBuilder.h"
#include "outputBuilders/CompetitionOutputBuilder.h"
#include "outputBuilders/DimacsOutputBuilder.h"

#include "heuristics/BerkminHeuristic.h"
#include "heuristics/FirstUndefinedHeuristic.h"
#include "heuristics/factories/BerkminCounterFactory.h"
#include "heuristics/factories/HeuristicCounterFactoryForLiteral.h"

void
WaspFacade::readInput()
{
    SATFormulaBuilder satFormulaBuilder( &solver );    
    Dimacs dimacs( &satFormulaBuilder );
    dimacs.parse();
}

void
WaspFacade::solve()
{
    if( printProgram )
    {
        solver.printProgram();
        return;
    }
    
    solver.init();
    if( !solver.preprocessing() )
    {
        solver.foundIncoherence();
        return;
    }
    
    while( solver.solve() )
    {
        solver.printAnswerSet();
        if( ++numberOfModels >= maxModels )
            break;
        else if( !solver.addClauseFromModelAndRestart() )
            break;
    }
    
    if( numberOfModels == 0 )
    {
        solver.foundIncoherence();
    }
}

void
WaspFacade::setDeletionPolicy(
    DELETION_POLICY deletionPolicy )
{
    switch( deletionPolicy )
    {
        case RESTARTS_BASED_DELETION_POLICY:
            solver.setDeletionStrategy( new RestartsBasedDeletionStrategy() );
            break;
            
        case MINISAT_DELETION_POLICY:
            solver.setDeletionStrategy( new MinisatDeletionStrategy() );
            break;

        case AGGRESSIVE_DELETION_POLICY:
        default:
            solver.setDeletionStrategy( new AggressiveDeletionStrategy() );
            break;
    }
}

void
WaspFacade::setHeuristicPolicy(
    HEURISTIC_POLICY heuristicPolicy,
    unsigned int heuristicLimit )
{
    switch( heuristicPolicy )
    {
        case HEURISTIC_BERKMIN:
            assert( heuristicLimit > 0 );
//            heuristicCounterFactoryForLiteral = new BerkminCounterFactory();    
            solver.setHeuristic( new BerkminHeuristic( heuristicLimit ) );
            break;
        
        case HEURISTIC_FIRST_UNDEFINED:
            solver.setHeuristic( new FirstUndefinedHeuristic() );
            break;
    
        default:
//            heuristicCounterFactoryForLiteral = new BerkminCounterFactory();    
            solver.setHeuristic( new BerkminHeuristic( 512 ) );
            break;
    }
}

void
WaspFacade::setOutputPolicy(
    OUTPUT_POLICY outputPolicy )
{
    switch( outputPolicy )
    {
        case COMPETITION_OUTPUT:
            solver.setOutputBuilder( new CompetitionOutputBuilder() );
            break;
            
        case DIMACS_OUTPUT:
            solver.setOutputBuilder( new DimacsOutputBuilder() );
            break;
            
        case SILENT_OUTPUT:
            solver.setOutputBuilder( new SilentOutputBuilder() );
            break;
            
        case THIRD_COMPETITION_OUTPUT:
            solver.setOutputBuilder( new ThirdCompetitionOutputBuilder() );
            break;
            
        case WASP_OUTPUT:
        default:
            solver.setOutputBuilder( new WaspOutputBuilder() );
            break;
    }
}

void
WaspFacade::setRestartsPolicy(
    RESTARTS_POLICY restartsPolicy,
    unsigned int threshold )
{
    assert( threshold > 0 );
    switch( restartsPolicy )
    {
        case SEQUENCE_BASED_RESTARTS_POLICY:
            solver.setRestartsStrategy( new SequenceBasedRestartsStrategy( threshold ) );
            break;
            
        case GEOMETRIC_RESTARTS_POLICY:
            solver.setRestartsStrategy( new GeometricRestartsStrategy( threshold ) );
            break;
            
        case MINISAT_RESTARTS_POLICY:
            solver.setRestartsStrategy( new MinisatRestartsStrategy( threshold ) );
            break;
            
        case NO_RESTARTS_POLICY:
            solver.setRestartsStrategy( new NoRestartsStrategy() );
            break;
            
        default:
            solver.setRestartsStrategy( new SequenceBasedRestartsStrategy( threshold ) );
            break;
    }
}
