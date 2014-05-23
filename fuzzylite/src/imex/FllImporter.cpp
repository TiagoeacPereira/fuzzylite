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
 * File:   FllImporter.cpp
 * Author: jcrada
 *
 * Created on 18 December 2013, 11:42 AM
 */

#include "fl/imex/FllImporter.h"

#include "fl/Headers.h"

#include <queue>

namespace fl {

    FllImporter::FllImporter(const std::string& separator) : _separator(separator) {
    }

    FllImporter::~FllImporter() {

    }

    std::string FllImporter::name() const {
        return "FllImporter";
    }

    void FllImporter::setSeparator(const std::string& separator) {
        this->_separator = separator;
    }

    std::string FllImporter::getSeparator() const {
        return this->_separator;
    }

    Engine* FllImporter::fromString(const std::string& fll) const {
        Engine* engine = new Engine;

        std::string tag;
        std::ostringstream block;
        std::istringstream fclReader(fll);
        std::string line;
        std::queue<std::string> lineQueue;

        try {
            bool processPending = false;
            int lineNumber = 0;
            while (not lineQueue.empty() or std::getline(fclReader, line)) {
                if (not lineQueue.empty()) {
                    line = lineQueue.front();
                    lineQueue.pop();
                } else {
                    line = clean(line);
                    if (line.empty()) continue;
                    std::vector<std::string> split = Op::split(line, _separator);
                    line = clean(split.front());
                    for (std::size_t i = 1; i < split.size(); ++i) {
                        lineQueue.push(clean(split.at(i)));
                    }
                    ++lineNumber;
                }
                if (line.empty()) continue;
                std::size_t colon = line.find_first_of(':');
                if (colon == std::string::npos) {
                    throw fl::Exception("[import error] expected a colon at line " +
                            Op::str(lineNumber) + ": " + line, FL_AT);
                }
                std::string key = Op::trim(line.substr(0, colon));
                std::string value = Op::trim(line.substr(colon + 1));
                if ("Engine" == key) {
                    engine->setName(value);
                    continue;
                } else {
                    processPending = (key == "InputVariable"
                            or key == "OutputVariable"
                            or key == "RuleBlock");
                }
                if (processPending) {
                    process(tag, block.str(), engine);
                    block.str(""); //clear buffer
                    block.clear(); //clear error flags
                    processPending = false;
                    tag = key;
                }
                block << key << ":" << value << "\n";
            }
            process(tag, block.str(), engine);
        } catch (std::exception& ex) {
            (void) ex;
            delete engine;
            throw;
        }
        return engine;
    }

    void FllImporter::process(const std::string& tag, const std::string& block, Engine* engine) const {
        if (tag.empty()) return;
        if ("InputVariable" == tag) {
            processInputVariable(block, engine);
        } else if ("OutputVariable" == tag) {
            processOutputVariable(block, engine);
        } else if ("RuleBlock" == tag) {
            processRuleBlock(block, engine);
        } else {
            throw fl::Exception("[import error] block tag <" + tag + "> not recognized", FL_AT);
        }
    }

    void FllImporter::processInputVariable(const std::string& block, Engine* engine) const {
        std::istringstream reader(block);
        std::string line;
        InputVariable* inputVariable = new InputVariable;
        engine->addInputVariable(inputVariable);
        while (std::getline(reader, line)) {
            std::pair<std::string, std::string> keyValue = parseKeyValue(line, ':');
            if ("InputVariable" == keyValue.first) {
                inputVariable->setName(keyValue.second);
            } else if ("enabled" == keyValue.first) {
                inputVariable->setEnabled(parseBoolean(keyValue.second));
            } else if ("range" == keyValue.first) {
                std::pair<scalar, scalar> range = parseRange(keyValue.second);
                inputVariable->setRange(range.first, range.second);
            } else if ("term" == keyValue.first) {
                inputVariable->addTerm(parseTerm(keyValue.second, engine));
            } else {
                throw fl::Exception("[import error] key <" + keyValue.first + "> not "
                        "recognized in pair <" + keyValue.first + ":" + keyValue.second + ">", FL_AT);
            }
        }
    }

    void FllImporter::processOutputVariable(const std::string& block, Engine* engine) const {
        std::istringstream reader(block);
        std::string line;
        OutputVariable* outputVariable = new OutputVariable;
        engine->addOutputVariable(outputVariable);
        while (std::getline(reader, line)) {
            std::pair<std::string, std::string> keyValue = parseKeyValue(line, ':');
            if ("OutputVariable" == keyValue.first) {
                outputVariable->setName(keyValue.second);
            } else if ("enabled" == keyValue.first) {
                outputVariable->setEnabled(parseBoolean(keyValue.second));
            } else if ("range" == keyValue.first) {
                std::pair<scalar, scalar> range = parseRange(keyValue.second);
                outputVariable->setRange(range.first, range.second);
            } else if ("default" == keyValue.first) {
                outputVariable->setDefaultValue(Op::toScalar(keyValue.second));
            } else if ("lock-previous" == keyValue.first) {
                outputVariable->setLockPreviousOutputValue(parseBoolean(keyValue.second));
            } else if ("lock-range" == keyValue.first) {
                outputVariable->setLockOutputValueInRange(parseBoolean(keyValue.second));
            } else if ("defuzzifier" == keyValue.first) {
                outputVariable->setDefuzzifier(parseDefuzzifier(keyValue.second));
            } else if ("accumulation" == keyValue.first) {
                outputVariable->fuzzyOutput()->setAccumulation(parseSNorm(keyValue.second));
            } else if ("term" == keyValue.first) {
                outputVariable->addTerm(parseTerm(keyValue.second, engine));
            } else {
                throw fl::Exception("[import error] key <" + keyValue.first + "> not "
                        "recognized in pair <" + keyValue.first + ":" + keyValue.second + ">", FL_AT);
            }
        }
    }

    void FllImporter::processRuleBlock(const std::string& block, Engine* engine) const {
        std::istringstream reader(block);
        std::string line;
        RuleBlock* ruleBlock = new RuleBlock;
        engine->addRuleBlock(ruleBlock);
        while (std::getline(reader, line)) {
            std::pair<std::string, std::string> keyValue = parseKeyValue(line, ':');
            if ("RuleBlock" == keyValue.first) {
                ruleBlock->setName(keyValue.second);
            } else if ("enabled" == keyValue.first) {
                ruleBlock->setEnabled(parseBoolean(keyValue.second));
            } else if ("conjunction" == keyValue.first) {
                ruleBlock->setConjunction(parseTNorm(keyValue.second));
            } else if ("disjunction" == keyValue.first) {
                ruleBlock->setDisjunction(parseSNorm(keyValue.second));
            } else if ("activation" == keyValue.first) {
                ruleBlock->setActivation(parseTNorm(keyValue.second));
            } else if ("rule" == keyValue.first) {
                Rule* rule = new Rule;
                rule->setText(keyValue.second);
                try {
                    rule->load(engine);
                } catch (std::exception& ex) {
                    FL_LOG(ex.what());
                }
                ruleBlock->addRule(rule);
            } else {
                throw fl::Exception("[import error] key <" + keyValue.first + "> not "
                        "recognized in pair <" + keyValue.first + ":" + keyValue.second + ">", FL_AT);
            }
        }
    }

    Term* FllImporter::parseTerm(const std::string& text, Engine* engine) const {
        std::vector<std::string> tokens = Op::split(text, " ");

        //MEDIUM Triangle 0.500 1.000 1.500

        if (tokens.size() < 2) {
            throw fl::Exception("[syntax error] expected a term in format <name class parameters>, "
                    "but found <" + text + ">", FL_AT);
        }
        Term* term = FactoryManager::instance()->term()->constructObject(tokens.at(1));
        term->setName(Op::makeValidId(tokens.at(0)));
        std::ostringstream parameters;
        for (std::size_t i = 2; i < tokens.size(); ++i) {
            parameters << tokens.at(i);
            if (i + 1 < tokens.size()) parameters << " ";
        }
        term->configure(parameters.str());
        //special cases:
        Linear* linear = NULL;
        Function* function = NULL;
        if ((linear = dynamic_cast<Linear*> (term))) {
            linear->setEngine(engine);
        } else if ((function = dynamic_cast<Function*> (term))) {
            function->setEngine(engine);
            //builtin functions are loaded from TermFactory calling Function::create
            function->load();
        }
        return term;
    }

    TNorm* FllImporter::parseTNorm(const std::string& name) const {
        if (name == "none") return FactoryManager::instance()->tnorm()->constructObject("");
        return FactoryManager::instance()->tnorm()->constructObject(name);
    }

    SNorm* FllImporter::parseSNorm(const std::string& name) const {
        if (name == "none") return FactoryManager::instance()->snorm()->constructObject("");
        return FactoryManager::instance()->snorm()->constructObject(name);
    }

    Defuzzifier* FllImporter::parseDefuzzifier(const std::string& text) const {
        std::vector<std::string> parameters = Op::split(text, " ");
        std::string name = parameters.at(0);
        if (name == "none") return FactoryManager::instance()->defuzzifier()->constructObject("");
        Defuzzifier* defuzzifier = FactoryManager::instance()->defuzzifier()->constructObject(name);
        if (parameters.size() > 1) {
            IntegralDefuzzifier* integralDefuzzifier =
                    dynamic_cast<IntegralDefuzzifier*> (defuzzifier);
            if (integralDefuzzifier) {
                integralDefuzzifier->setResolution((int) Op::toScalar(parameters.at(1)));
            }
        }
        return defuzzifier;
    }

    std::pair<scalar, scalar> FllImporter::parseRange(const std::string& text) const {
        std::pair<std::string, std::string> range = parseKeyValue(text, ' ');
        return std::pair<scalar, scalar>(Op::toScalar(range.first), Op::toScalar(range.second));
    }

    bool FllImporter::parseBoolean(const std::string& boolean) const {
        if ("true" == boolean) return true;
        if ("false" == boolean) return false;
        throw fl::Exception("[syntax error] expected boolean <true|false>, "
                "but found <" + boolean + ">", FL_AT);
    }

    std::pair<std::string, std::string> FllImporter::parseKeyValue(const std::string& text,
            char separator) const {
        std::size_t half = text.find_first_of(separator);
        if (half == std::string::npos) {
            std::ostringstream ex;
            ex << "[syntax error] expected pair in the form "
                    "<key" << separator << "value>, but found <" << text << ">";
            throw fl::Exception(ex.str(), FL_AT);
        }
        std::pair<std::string, std::string> result;
        result.first = text.substr(0, half);
        result.second = text.substr(half + 1);
        return result;
    }

    std::string FllImporter::clean(const std::string& line) const {
        if (line.empty()) return line;
        if (line.size() == 1) return isspace(line.at(0)) ? "" : line;
        int start = 0, end = line.size() - 1;
        while (start <= end and isspace(line.at(start))) {
            ++start;
        }
        int sharp = start;
        while (sharp <= end) {
            if (line.at(sharp) == '#') {
                end = sharp - 1;
                break;
            }
            ++sharp;
        }
        while (end >= start and (line.at(end) == '#' or isspace(line.at(end)))) {
            --end;
        }

        int length = end - start + 1;
        return line.substr(start, length);
    }

    FllImporter* FllImporter::clone() const {
        return new FllImporter(*this);
    }


}
