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

#ifndef ABSTRACTRULE_H
#define ABSTRACTRULE_H

class AbstractRule
{
    public:
        AbstractRule() : id_( 0 ), choice_( false ) {}
        AbstractRule( unsigned int id, bool choice ) : id_( id ), choice_( choice ) {}
        inline unsigned int getId() const { return id_; }
        inline void setId( unsigned int id ) { id_ = id; }
        inline bool isChoice() const { return choice_; }
        inline void setChoice( bool choice ) { choice_ = choice; } 
    
    private:
        unsigned int id_ : 31;
        unsigned int choice_ : 1;
};

#endif