#pragma once

#include <cmath>
#include <regex>
#include <unordered_set>
#include <set>

#include "common.h"
#include "formula.h"

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    enum class CellType { 
        ct_empty, 
        ct_formula, 
        ct_text 
    };
    CellType ParseCellType(const std::string& text);

    void CheckCircularDependency(const std::vector<Position>& current_ref_cells);


    class Impl {
    public:
        explicit Impl(SheetInterface& sheet);
        virtual ~Impl() = default;

        virtual void Set(std::string&& text) = 0;
        virtual const std::string& GetText() const;
        virtual const Value GetValue() = 0;
        virtual std::vector<Position> GetReferencedCells() const;

    protected:
        SheetInterface& sheet_;
        mutable std::string text_;
    };


    class EmptyImpl : public Impl {
    public:
        explicit EmptyImpl(SheetInterface& sheet, std::string&& text);
        void Set(std::string&& text) override;
        const Value GetValue() override;
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(SheetInterface& sheet, std::string&& text);
        void Set(std::string&& text) override;
        const Value GetValue() override;
    };

    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(SheetInterface& sheet, std::string&& text);
        void Set(std::string&& text) override;
        const Value GetValue() override;
        const std::string& GetText() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        std::unique_ptr<FormulaInterface> formula_;
    };

    SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_;
    std::unordered_set<const Cell*> dependent_cells_;
};