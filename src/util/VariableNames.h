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

#ifndef VARIABLENAMES_H
#define VARIABLENAMES_H

#include <cassert>
#include <iostream>
#include <unordered_map>
using namespace std;

class Variable;

class VariableNames
{
    public:

        static bool isHidden( const Variable* variable );
        static const string& getName( const Variable* variable );
        static void setName( const Variable* variable, string name );
        
    private:        
        static unordered_map< const Variable*, string > variables;
};

#endif
