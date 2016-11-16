#include "ReductBasedCheck.h"
#include "Solver.h"
#include "input/Dimacs.h"
#include "outputBuilders/DimacsOutputBuilder.h"
#include "outputBuilders/WaspOutputBuilder.h"
#include "util/Trace.h"
#include "util/Options.h"

#include <vector>
#include "util/Options.h"

ostream& operator<<( ostream& out, const ReductBasedCheck& component )
{
    out << "[ ";
    for( unsigned i = 0; i < component.hcVariables.size(); ++i )
        out << component.solver.getLiteral( component.hcVariables[ i ] ) << " ";
    return out << "]";
}

ReductBasedCheck::ReductBasedCheck(
    vector< GUSData* >& gusData_,
    Solver& s,
    unsigned numberOfInputAtoms ) : HCComponent( gusData_, s ),
        numberOfAtoms( numberOfInputAtoms ), 
        assumptionLiteral( Literal::null ), isConflictual( false ),
        numberOfExternalLiterals( 0 ), numberOfInternalVariables( 0 ), numberOfZeroLevel( 0 ), removedHCVars( 0 ), literalToAdd( Literal::null ),
        low( 0 ), high( solver.numberOfVariables() )
{   
    inUnfoundedSet.push_back( 0 );
    generatorToCheckerId.push_back( UINT_MAX );
    while( checker.numberOfVariables() < numberOfInputAtoms )
    {
        inUnfoundedSet.push_back( 0 );
        checker.addVariable();
        generatorToCheckerId.push_back( UINT_MAX );
    }
        
    assert_msg( checker.numberOfVariables() == numberOfInputAtoms, checker.numberOfVariables() << " != " << numberOfInputAtoms );
    assert( checker.numberOfVariables() == inUnfoundedSet.size() - 1 );
    
    checker.initFrom( solver );
    checker.setChecker(); 
    checker.disableStatistics();
    checker.disableVariableElimination();
}

ReductBasedCheck::~ReductBasedCheck()
{
    checker.enableStatistics();
    statistics( &checker, onDeletingChecker( id ) );    
}

bool
ReductBasedCheck::addExternalLiteral(
    Literal lit ) 
{    
    if( lit.isPositive() )
    {
        if( inUnfoundedSet[ lit.getVariable() ] & 1 )
            return false;
        inUnfoundedSet[ lit.getVariable() ] |= 1;
    }
    else
    {
        assert( lit.isNegative() );
        if( inUnfoundedSet[ lit.getVariable() ] & 2 )
            return false;
        inUnfoundedSet[ lit.getVariable() ] |= 2;
        checker.addVariable();
        generatorToCheckerId[ lit.getVariable() ] = checker.numberOfVariables();    
    }    
    externalLiterals.push_back( lit );    
    numberOfExternalLiterals++;
    return true;
}

bool
ReductBasedCheck::addExternalLiteralForInternalVariable(
    Literal lit )
{
    assert( lit.isPositive() );
    if( inUnfoundedSet[ lit.getVariable() ] & 2 )
        return false;
    inUnfoundedSet[ lit.getVariable() ] |= 2;
    checker.addVariable();
    generatorToCheckerId[ lit.getVariable() ] = checker.numberOfVariables();
    externalLiterals.push_back( lit );
    numberOfExternalLiterals++;
    return true;
}

void
ReductBasedCheck::reset()
{    
    while( !trail.empty() && solver.isUndefined( trail.back() ) )
    {
        hasToTestModel = false;
        trail.pop_back();
    }

    unfoundedSet.clear();
}

bool
ReductBasedCheck::onLiteralFalse(
    Literal literal )
{    
    if( solver.getCurrentDecisionLevel() > 0 )
        trail.push_back( literal );
    else
        numberOfZeroLevel++;
    
    if( wasp::Options::forwardPartialChecks && ( low + high ) / 2 <= solver.getCurrentDecisionLevel() )
    {
        hasToTestModel = true;
        return true;
    }
    
    if( trail.size() + numberOfZeroLevel == ( numberOfInternalVariables + numberOfExternalLiterals ) )
    {
        hasToTestModel = true;
        return true;
    }
    
    return false;
}

void
ReductBasedCheck::testModel()
{
    trace_msg( modelchecker, 1, "Check component " << *this );
    if( numberOfCalls++ == 0 )
        initDataStructures();    
    
    //The checker will return always unsat
    if( isConflictual )
        return;

    vector< Literal > assumptions;
    computeAssumptions( assumptions );
    //The checker will return always unsat
    if( isConflictual )
        return;
    
    if( unfoundedSetCandidates.empty() )
        return;
    
    checkModel( assumptions );    
    
    clearUnfoundedSetCandidates();        

    assert( !checker.conflictDetected() );
    if( assumptionLiteral != Literal::null )
        checker.addClause( assumptionLiteral.getOppositeLiteral() );
    assert( !checker.conflictDetected() );
    statistics( &checker, endCheckerInvocation( time( 0 ) ) );
}

void
ReductBasedCheck::computeAssumptions(
    vector< Literal >& assumptions )
{
    trace_msg( modelchecker, 1, "Computing assumptions" );
    assert_msg( unfoundedSetCandidates.size() == removedHCVars, unfoundedSetCandidates.size() << "!=" << removedHCVars );
    iterationInternalLiterals( assumptions );
    iterationExternalLiterals( assumptions );        
}

void
ReductBasedCheck::iterationInternalLiterals(
    vector< Literal >& assumptions )
{
    bool hasToAddClause = true;
    Clause* clause = new Clause();    
    for( unsigned int i = 0; i < hcVariables.size(); i++ )
    {
        Literal lit = Literal( hcVariables[ i ], NEGATIVE );
        if( solver.isFalse( hcVariables[ i ] ) )
            assumptions.push_back( lit );            
        else
        {
            unfoundedSetCandidates.push_back( lit );
            if( !hasToAddClause )
                continue;
            hasToAddClause = addLiteralInClause( lit, clause );
        }
    }
    
    if( hasToAddClause && literalToAdd != Literal::null )
        hasToAddClause = addLiteralInClause( literalToAdd, clause );
    
    if( !hasToAddClause )
    {
        delete clause;
        return;
    }
    
    Var addedVar = addFreshVariable();
    assumptionLiteral = Literal( addedVar, POSITIVE );
    clause->addLiteral( assumptionLiteral.getOppositeLiteral() );
    assumptions.push_back( assumptionLiteral );

    trace_msg( modelchecker, 2, "Adding clause " << *clause );
    #ifndef NDEBUG
    bool result =
    #endif
    checker.addClauseRuntime( clause );
    assert( result );
    statistics( &checker, assumptionsOR( clause->size() ) );            
    trace_msg( modelchecker, 3, "Adding " << assumptionLiteral << " as assumption" );    
}
void

ReductBasedCheck::iterationExternalLiterals(
    vector< Literal >& assumptions )
{
    int j = 0;
    for( unsigned int i = 0; i < externalLiterals.size(); i++ )
    {
        Literal lit = externalLiterals[ j ] = externalLiterals[ i ];
        assert( getCheckerVarFromExternalLiteral( lit ) != UINT_MAX );
        if( solver.getDecisionLevel( lit ) > 0 || solver.isUndefined( lit ) )
        {
            assumptions.push_back( Literal( getCheckerVarFromExternalLiteral( lit ), solver.isTrue( lit ) ? POSITIVE : NEGATIVE ) );
            j++;
        }
        else
        {
            isConflictual = !checker.addClauseRuntime( Literal( getCheckerVarFromExternalLiteral( lit ), solver.isTrue( lit ) ? POSITIVE : NEGATIVE ) );
            if( isConflictual )
                return;
        }
    }
    externalLiterals.resize( j );
    
    statistics( &checker, assumptions( assumptions.size() ) );
}

void
ReductBasedCheck::addClauseToChecker(
    Clause* c,
    Var headAtom )
{
    assert( c != NULL );
    trace_msg( modelchecker, 2, "Adding clause " << *c );
    Clause& orig = *c;
    Clause* clause = new Clause( c->size() );    
    for( unsigned int i = 0; i < orig.size(); i++ )
    {        
        clause->addLiteral( orig[ i ] );        
        Var v = orig[ i ].getVariable();
        if( !sameComponent( v ) )
        {
            Var newVar = getCheckerVarFromExternalLiteral( orig[ i ] );            
            if( newVar != v )
            {
                orig[ i ].setVariable( newVar );
                orig[ i ].setPositive();
            }
        }
        else
        {
            if( orig[ i ].isNegativeBodyLiteral() )
            {
                Var newVar = getCheckerVarFromExternalLiteral( orig[ i ] );
                if( newVar != v )
                {
                    orig[ i ].setVariable( newVar );
                    orig[ i ].setPositive();
                }
            }
        }
    }    
    getGUSData( headAtom ).definingRulesForNonHCFAtom.push_back( clause );    
    checker.addClause( c );    
}

Clause*
ReductBasedCheck::getClauseToPropagate(
    Learning& learning )
{
    assert( unfoundedSet.empty() );
    if( hasToTestModel )
        testModel();

    hasToTestModel = false;
    if( !unfoundedSet.empty() )
    {
        trace_msg( modelchecker, 1, "Learning unfounded set rule for component " << *this );
        Clause* loopFormula = learning.learnClausesFromDisjunctiveUnfoundedSet( unfoundedSet );
        trace_msg( modelchecker, 1, "Adding loop formula: " << *loopFormula );
        unfoundedSet.clear();
        solver.onLearningALoopFormulaFromModelChecker();    
        return loopFormula;
    }
    return NULL;
}

void
ReductBasedCheck::createInitialClauseAndSimplifyHCVars()
{
    trace_msg( modelchecker, 1, "Simplifying Head Cycle variables" );
    Clause* clause = new Clause();

    bool satisfied = false;
    int j = 0;
    for( unsigned int i = 0; i < hcVariables.size(); i++ )
    {
        Var v = hcVariables[ j ] = hcVariables[ i ];
        if( solver.isTrue( v ) && solver.getDecisionLevel( v ) == 0 )
        {
            Literal lit = Literal( v, NEGATIVE );
            unfoundedSetCandidates.push_back( lit );            
            removedHCVars++;
            trace_msg( modelchecker, 2, "Variable " << Literal( v ) << " is true at level 0: removed" );
            if( !satisfied )
                satisfied = !addLiteralInClause( lit, clause );
        }
        else
            j++;
    }
    hcVariables.resize( j );        
    
    trace_msg( modelchecker, 2, "Clause before adding a fresh variable " << *clause << ", which is " << ( satisfied ? "" : "not" ) << " satisfied");
    if( clause->size() == 0 || satisfied )
    {
        trace_msg( modelchecker, 3, "Removed" );
        delete clause;
    }
    else
    {
        statistics( &checker, setTrueAtLevelZero( removedHCVars ) );
        assert( clause != NULL );
        Var newVar = addFreshVariable();
        literalToAdd = Literal( newVar, NEGATIVE );
        clause->addLiteral( literalToAdd.getOppositeLiteral() );
        
        trace_msg( modelchecker, 3, "Clause to add " << *clause );
        if( !isConflictual )
            isConflictual = !checker.addClauseRuntime( clause );        
    }    
    
    assert( removedHCVars == unfoundedSetCandidates.size() );
}

void
ReductBasedCheck::clearUnfoundedSetCandidates()
{
    unfoundedSetCandidates.shrink( removedHCVars );
}

Var
ReductBasedCheck::addFreshVariable()
{
    checker.addVariableRuntime();
    return checker.numberOfVariables();
}

void
ReductBasedCheck::initDataStructures()
{
    trace_msg( modelchecker, 2, "First call. Removing unused variables" );
    for( unsigned i = 1; i < inUnfoundedSet.size(); ++i )
    {
        if( inUnfoundedSet[ i ] == 0 )
            checker.addClause( Literal( i, POSITIVE ) );
        else
            inUnfoundedSet[ i ] = 0;        
    }
    #ifndef NDEBUG
    bool result = 
    #endif    
    checker.preprocessing();
    assert( result );

    checker.turnOffSimplifications();
    createInitialClauseAndSimplifyHCVars();           
}

void ReductBasedCheck::checkModel(
    vector< Literal >& assumptions )
{    
    assert( !checker.conflictDetected() );
    assert( checker.getCurrentDecisionLevel() == 0 );
    trace_action( modelchecker, 2, { printVector( assumptions, "Assumptions" ); } );
    trace_action( modelchecker, 2, { printVector( unfoundedSetCandidates, "Unfounded set candidates" ); } );
    statistics( &checker, startCheckerInvocation( trail.size() != ( hcVariables.size() + externalLiterals.size() ), time( 0 ) ) );
    checker.clearConflictStatus();
    
    if( checker.solve( assumptions ) == COHERENT )
    {
        trace_msg( modelchecker, 1, "SATISFIABLE: the model is not stable." );
        for( unsigned int i = 0; i < unfoundedSetCandidates.size(); i++ )
            if( checker.isTrue( unfoundedSetCandidates[ i ] ) )
            {
                Var curr = unfoundedSetCandidates[ i ].getVariable();
                unfoundedSet.push_back( curr );
                setInUnfoundedSet( curr );
            }
        trace_action( modelchecker, 2, { printVector( unfoundedSet, "Unfounded set" ); } );
        statistics( &checker, foundUS( trail.size() != ( hcVariables.size() + externalLiterals.size() ), unfoundedSet.size() ) );
        assert( !unfoundedSet.empty() );
        high = solver.getCurrentDecisionLevel();        
    }
    else
    {
        trace_msg( modelchecker, 1, "UNSATISFIABLE: the model is stable." );
        low = solver.getCurrentDecisionLevel();        
        checker.clearConflictStatus();        
    }
    
    assert( !checker.conflictDetected() );
    checker.unrollToZero();    
}

void ReductBasedCheck::addHCVariableProtected(
    Var v )
{
    inUnfoundedSet[ v ] |= 4;
    numberOfInternalVariables++;
    attachLiterals( Literal( v, POSITIVE ) );
    solver.setComponent( v, NULL );
    solver.setHCComponent( v, this );
}

void ReductBasedCheck::processRule(
    Rule* rule,
    Var headAtom )
{
    Clause* c = new Clause();
    for( unsigned int k = 0; k < rule->size(); k++ )
    {
        Literal lit = rule->literals[ k ];
        c->addLiteral( lit );
        if( ( !sameComponent( lit.getVariable() ) && addExternalLiteral( lit ) ) || ( lit.isNegativeBodyLiteral() && addExternalLiteralForInternalVariable( lit ) ) )
            attachLiterals( lit );        
    }
    c->removeDuplicates();
    addClauseToChecker( c, headAtom );  
}

void ReductBasedCheck::processComponentBeforeStarting()
{
    for( unsigned int j = 0; j < size(); j++ )
    {
        Var v = getVariable( j );
        if( solver.isTrue( v ) )
            onLiteralFalse( Literal( v, NEGATIVE ) );
        else if( solver.isFalse( v ) )
            onLiteralFalse( Literal( v, POSITIVE ) );
    }

    for( unsigned int j = 0; j < externalLiteralsSize(); j++ )
    {
        Literal lit = getExternalLiteral( j );
        if( solver.isTrue( lit ) )
            onLiteralFalse( lit.getOppositeLiteral() );
        else if( solver.isFalse( lit ) )
            onLiteralFalse( lit );
    }
}