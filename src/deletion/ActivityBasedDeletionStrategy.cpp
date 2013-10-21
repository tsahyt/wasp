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

#include "ActivityBasedDeletionStrategy.h"
#include "../LearnedClause.h"
#include "../Solver.h"

bool
ActivityBasedDeletionStrategy::hasToDeleteClauseThreshold(
    LearnedClause* clause )
{
    Activity activity = clause->getActivity();
    
    bool hasToDelete = false;
    if( !clause->isLocked() )
    {
        activitySum += activity;
        ++activityCount;
        if ( activity < threshold )
        {
            toDelete--;
            hasToDelete = true;
        }        
    }
        
    return hasToDelete;
}

bool
ActivityBasedDeletionStrategy::hasToDeleteClauseAvg(
    LearnedClause* clause )
{
    Activity activity = clause->getActivity();
    
    bool hasToDelete = false;
    if( !clause->isLocked() )
    {
        if ( activity < activitySum )
        {
            toDelete--;
            hasToDelete = true;
        }        
    }
        
    return hasToDelete;
}

void
ActivityBasedDeletionStrategy::updateActivity( 
    LearnedClause* learnedClause )
{
    learnedClause->incrementActivity( increment );
    decrementActivity();
    if( learnedClause->getActivity() > 1e20 )
    {
        solver.decreaseLearnedClausesActivity();
        increment *= 1e-20;
    }
}

void
ActivityBasedDeletionStrategy::deleteClauses()
{
    startIteration();
    
    for( typename Solver::LearnedClausesIterator it = solver.learnedClauses_begin(); it != solver.learnedClauses_end(); )
    {
        typename Solver::LearnedClausesIterator tmp_it = it++;
        LearnedClause* currentClause = *tmp_it;
        if( hasToDeleteClauseThreshold( currentClause ) )
        {
            solver.deleteLearnedClause( currentClause, tmp_it );            
        }
    }
    
    if( activityCount > 0 && toDelete > 0 )
    {
        activitySum = activitySum / activityCount;
        
        for( typename Solver::LearnedClausesIterator it = solver.learnedClauses_begin(); it != solver.learnedClauses_end(); )
        {
            typename Solver::LearnedClausesIterator tmp_it = it++;
            LearnedClause* currentClause = *tmp_it;
            if( hasToDeleteClauseAvg( currentClause ) )
            {
                solver.deleteLearnedClause( currentClause, tmp_it );            
            }
        }
    }
    
}

void
ActivityBasedDeletionStrategy::startIteration()
{
    activitySum = 0;
    activityCount = 0;
    threshold = increment / solver.numberOfLearnedClauses();        
    toDelete = solver.numberOfLearnedClauses() / 2;
}