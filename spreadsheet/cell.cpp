#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>


Cell::Impl::Impl(SheetInterface& sheet)
    : sheet_(sheet), text_() {}

const std::string& Cell::Impl::GetText() const {
    return text_;
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

// EmptyImpl implementation
Cell::EmptyImpl::EmptyImpl(SheetInterface& sheet, std::string&& text)
    : Impl(sheet) {}

void Cell::EmptyImpl::Set(std::string&& text) {
    
}

const CellInterface::Value Cell::EmptyImpl::GetValue() {
    return 0.0;  
}


Cell::TextImpl::TextImpl(SheetInterface& sheet, std::string&& text)
    : Impl(sheet) {
    Set(std::move(text));
}

void Cell::TextImpl::Set(std::string&& text) {
    text_ = std::move(text);
}

const CellInterface::Value Cell::TextImpl::GetValue() {
    if (!text_.empty() && text_[0] == ESCAPE_SIGN) {
        return text_.substr(1);  
    }

    
    std::regex rx(R"(^([+-]?(?:\d+\.?|\d*\.\d+))(?:[Ee][+-]?\d+)?$)");
    if (std::regex_match(text_, rx)) {
        try {
            return std::stod(text_);
        } catch (...) {
            
        }
    }
    return text_;
}


Cell::FormulaImpl::FormulaImpl(SheetInterface& sheet, std::string&& text)
    : Impl(sheet) {
    Set(std::move(text));
}

void Cell::FormulaImpl::Set(std::string&& text) {
    formula_ = ParseFormula(text.substr(1)); 


    for (const auto& pos : formula_->GetReferencedCells()) {
        if (!sheet_.GetCell(pos)) {
            sheet_.SetCell(pos, "");
        }
    }
}

const CellInterface::Value Cell::FormulaImpl::GetValue() {
    try {
        FormulaInterface::Value result = formula_->Evaluate(sheet_);
        if (std::holds_alternative<double>(result)) {
            return std::get<double>(result);
        } else {
            return std::get<FormulaError>(result);
        }
    } catch (...) {
        throw;
    }
}

const std::string& Cell::FormulaImpl::GetText() const {
    text_ = '=' + formula_->GetExpression();
    return Impl::GetText();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

Cell::Cell(SheetInterface& sheet)
    : sheet_(sheet), impl_(std::make_unique<EmptyImpl>(sheet_, "")) {}

Cell::~Cell() {
    Clear();
}

void Cell::Set(std::string text) {
    CellType new_cell_type = ParseCellType(text);
    std::unique_ptr<Impl> new_impl;

    if (new_cell_type == CellType::ct_empty) {
        new_impl = std::make_unique<EmptyImpl>(sheet_, std::move(text));
    } else if (new_cell_type == CellType::ct_formula) {
        new_impl = std::make_unique<FormulaImpl>(sheet_, std::move(text));
        CheckCircularDependency(new_impl->GetReferencedCells());
    } else {
        new_impl = std::make_unique<TextImpl>(sheet_, std::move(text));
    }

    impl_ = std::move(new_impl);
}

void Cell::Clear() {
    impl_.reset(nullptr);
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

Cell::CellType Cell::ParseCellType(const std::string& text) {
    if (text.empty()) {
        return CellType::ct_empty;
    } else if (text.at(0) == FORMULA_SIGN && text.length() > 1) {
        return CellType::ct_formula;
    } else {
        return CellType::ct_text;
    }
}

void Cell::CheckCircularDependency(const std::vector<Position>& current_ref_cells) {
    if (current_ref_cells.empty()) {
        return;
    }

    std::set<const CellInterface*> stack;
    std::set<const CellInterface*> visited{this};

    auto add_ref_cells = [&stack](const std::vector<Position>& ref_cells, const SheetInterface& sheet) {
        for (const auto& pos : ref_cells) {
            const CellInterface* cell = sheet.GetCell(pos);
            if (cell) {
                stack.insert(cell);
            }
        }
    };

    add_ref_cells(current_ref_cells, sheet_);

    while (!stack.empty()) {
        const CellInterface* last_cell = *stack.begin();
        stack.erase(stack.begin());

        if (visited.find(last_cell) != visited.end()) {
            throw CircularDependencyException("");
        }

        visited.insert(last_cell);
        add_ref_cells(last_cell->GetReferencedCells(), sheet_);
    }
}