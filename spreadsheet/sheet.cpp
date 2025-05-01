#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {

}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }

    std::size_t row = static_cast<std::size_t>(pos.row);
    std::size_t col = static_cast<std::size_t>(pos.col);

    if (row >= data_.size()) {
        data_.resize(row + 1);
    }

    std::size_t max_cols = std::max(col + 1, data_[0].size());
    size_.cols = max_cols;
    size_.rows = data_.size();

    for (auto& current_row : data_) {
        if (current_row.size() < max_cols) {
            current_row.resize(max_cols);
        }
    }

    auto& cell = data_[row][col];
    if (!cell) {
        cell = std::make_unique<Cell>(*this);
    }
    cell->Set(text);

}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }

    std::size_t row = static_cast<std::size_t>(pos.row);
    std::size_t col = static_cast<std::size_t>(pos.col);

    return (row >= data_.size() || col >= data_[0].size()) ? nullptr : data_[row][col].get();
}

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface*>(static_cast<const Sheet&>(*this).GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!GetCell(pos)) {
        return;
    }

    std::size_t row = static_cast<std::size_t>(pos.row);
    std::size_t col = static_cast<std::size_t>(pos.col);

    auto& cell = data_[row][col];
    if (cell) {
        cell->Clear();
        cell.reset(nullptr);
        if (pos.row == size_.rows - 1 || pos.col == size_.cols - 1) {
            ComputeSize();
        }
    }
}

void Sheet::ComputeSize() {
    size_ = { 0,0 };
    if (data_.empty()) {
        return;
    }

    for (std::size_t row = 0; row < data_.size(); ++row) {
        std::size_t row_size = data_[row].size();
        for (std::size_t col = 0; col < row_size; ++col) {
            const auto& cell = data_[row][col];
            if (cell && !cell->GetText().empty()) {
                size_.cols = std::max(size_.cols, static_cast<int>(col + 1));
                size_.rows = std::max(size_.rows, static_cast<int>(row + 1));
            }
        }
    }

    if (data_.size() > static_cast<std::size_t>(size_.rows)) {
        data_.resize(size_.rows);
    }

    if (!data_.empty() && data_[0].size() > static_cast<std::size_t>(size_.cols)) {
        for (auto& row : data_) {
            row.resize(size_.cols);
        }
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (const auto& row : data_) {
        for (std::size_t col = 0; col < row.size(); ++col) {
            auto& cell = row[col];
            if (cell) {
                auto value = cell->GetValue();
                if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                }
                else if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                }
                else {
                    output << std::get<FormulaError>(value);
                }
            }
            if (col < row.size() - 1) {
                output << "\t";
            }
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (const auto& row : data_) {
        for (std::size_t col = 0; col < row.size(); ++col) {
            auto& cell = row[col];
            if (cell) {
                output << cell->GetText();
            }
            if (col < row.size() - 1) {
                output << "\t";
            }
        }
        output << "\n";
    }
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    return dynamic_cast<const Cell*>(GetCell(pos));
}

Cell* Sheet::GetConcreteCell(Position pos) {
    return dynamic_cast<Cell*>(GetCell(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}