#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

std::ostream& operator<<(std::ostream& output, FormulaException fe) {
    return output << fe.what();
}

namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression) try
            : ast_(ParseFormulaAST(expression)) {
        }
        catch (const std::exception& e) {
            throw FormulaException(e.what());
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                return ast_.Execute(sheet);
            }
            catch (const FormulaError& e) {
                return e;
            }
        }

        std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const {
            std::set<Position> tmp(ast_.GetCells().begin(), ast_.GetCells().end());
            return {tmp.begin(),tmp.end()};
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}