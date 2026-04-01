#pragma once
#include "lexer.hpp"

#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace ast {

struct Node {
    virtual ~Node() = default;
    virtual void print(int indent = 0) const = 0;
};

struct Expression : public Node {
    virtual ~Expression() = default;
    virtual std::string toString() const = 0;
};

struct ColumnRef : public Expression {
    std::string table_name;
    std::string column_name;

    ColumnRef(const std::string& t, const std::string& c) : table_name(t), column_name(c) {}

    void print(int indent = 0) const override {
        std::string pad(indent, ' ');
        if (table_name.empty()) {
            std::cout << pad << "- ColumnRef: " << column_name << "\n";
        } else {
            std::cout << pad << "- ColumnRef: " << table_name << "." << column_name << "\n";
        }
    }
    
    std::string toString() const override {
        return table_name.empty() ? column_name : table_name + "." + column_name;
    }
};

struct Literal : public Expression {
    std::string value;
    bool is_number;

    Literal(const std::string& v, bool is_num) : value(v), is_number(is_num) {}

    void print(int indent = 0) const override {
        std::string pad(indent, ' ');
        std::cout << pad << "- Literal: " << (is_number ? value : ("'" + value + "'")) << "\n";
    }
    
    std::string toString() const override {
        return is_number ? value : "'" + value + "'";
    }
};

struct BinaryExpr : public Expression {
    std::string op;
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;

    BinaryExpr(const std::string& o, std::shared_ptr<Expression> l, std::shared_ptr<Expression> r)
        : op(o), left(l), right(r) {}

    void print(int indent = 0) const override {
        std::string pad(indent, ' ');
        std::cout << pad << "- BinaryExpr (" << op << ")\n";
        left->print(indent + 2);
        right->print(indent + 2);
    }
    
    std::string toString() const override {
        return left->toString() + " " + op + " " + right->toString();
    }
};

struct Source : public Node {
    virtual ~Source() = default;
};

struct TableRef : public Source {
    std::string table_name;
    std::string alias;

    TableRef(const std::string& name, const std::string& a = "") : table_name(name), alias(a) {}

    void print(int indent = 0) const override {
        std::string pad(indent, ' ');
        std::cout << pad << "- TableRef: " << table_name;
        if (!alias.empty()) std::cout << " AS " << alias;
        std::cout << "\n";
    }
};

struct JoinData {
    std::shared_ptr<Source> left;
    std::shared_ptr<Source> right;
    std::shared_ptr<Expression> condition;
    std::string join_type;
};

struct JoinNode : public Source {
    JoinData join_info;

    JoinNode(JoinData d) : join_info(std::move(d)) {}

    void print(int indent = 0) const override {
        std::string pad(indent, ' ');
        std::cout << pad << "- Join (" << join_info.join_type << ")\n";
        std::cout << pad << "  Left:\n";
        join_info.left->print(indent + 4);
        std::cout << pad << "  Right:\n";
        join_info.right->print(indent + 4);
        if (join_info.condition) {
            std::cout << pad << "  ON:\n";
            join_info.condition->print(indent + 4);
        }
    }
};

struct SelectStatement : public Node {
    std::vector<std::shared_ptr<Expression>> select_list;
    std::shared_ptr<Source> from_source;
    std::shared_ptr<Expression> where_clause;

    void print(int indent = 0) const override {
        std::string pad(indent, ' ');
        std::cout << pad << "Ast: SelectStatement\n";
        std::cout << pad << "  SELECT:\n";
        for (const auto& expr : select_list) expr->print(indent + 4);
        if (from_source) {
            std::cout << pad << "  FROM:\n";
            from_source->print(indent + 4);
        }
        if (where_clause) {
            std::cout << pad << "  WHERE:\n";
            where_clause->print(indent + 4);
        }
    }
};

} // namespace ast
