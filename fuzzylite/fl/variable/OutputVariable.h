// #BEGIN_LICENSE
// fuzzylite: a fuzzy logic control library in C++
// Copyright (C) 2014  Juan Rada-Vilela
// 
// This file is part of fuzzylite.
//
// fuzzylite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// fuzzylite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fuzzylite.  If not, see <http://www.gnu.org/licenses/>.
// #END_LICENSE

/*
 * OutputVariable.h
 *
 *  Created on: 2/12/2012
 *      Author: jcrada
 */

#ifndef FL_OUTPUTVARIABLE_H
#define FL_OUTPUTVARIABLE_H

#include "fl/variable/Variable.h"

#include <limits>
#include <memory>

namespace fl {
    class Accumulated;
    class Defuzzifier;

    class FL_EXPORT OutputVariable : public Variable {
    private:
        void copyFrom(const OutputVariable& rhs);
    protected:
        Accumulated* _fuzzyOutput;
        std::auto_ptr<Defuzzifier> _defuzzifier;
        scalar _outputValue;
        scalar _previousOutputValue;
        scalar _defaultValue;
        bool _lockOutputValueInRange;
        bool _lockPreviousOutputValue;

    public:
        OutputVariable(const std::string& name = "",
                scalar minimum = -fl::inf, scalar maximum = fl::inf);
        OutputVariable(const OutputVariable& copy);
        OutputVariable& operator=(const OutputVariable& rhs);
        virtual ~OutputVariable();

        virtual Accumulated* fuzzyOutput() const;

        virtual void setName(const std::string& name);

        virtual void setMinimum(scalar minimum);
        virtual void setMaximum(scalar maximum);

        virtual void setDefuzzifier(Defuzzifier* defuzzifier);
        virtual Defuzzifier* getDefuzzifier() const;

        virtual void setOutputValue(scalar outputValue);
        virtual scalar getOutputValue() const;

        virtual void setPreviousOutputValue(scalar defuzzifiedValue);
        virtual scalar getPreviousOutputValue() const;

        virtual void setDefaultValue(scalar defaultValue);
        virtual scalar getDefaultValue() const;

        virtual void setLockOutputValueInRange(bool lockOutputValueInRange);
        virtual bool isLockedOutputValueInRange() const;
        
        virtual void setLockPreviousOutputValue(bool lockPreviousOutputValue);
        virtual bool isLockedPreviousOutputValue() const;

        virtual void defuzzify();

        virtual std::string fuzzyOutputValue() const;

        virtual void clear();

        virtual std::string toString() const;

    };

}
#endif /* FL_OUTPUTVARIABLE_H */
