#pragma once
#include "ast.hpp"
#include <memory>
#include <vector>
#include <string>

namespace logical {

struct LogicalNode {
    virtual ~LogicalNode() = default;
    virtual std::string toJson() const = 0;
    
    // NEW: Translates the Relational Algebra back into SQL!
    virtual std::string toSql() const = 0; 
};

struct LogicalScan : public LogicalNode {
    std::string table_name;
    std::string alias;
    
    LogicalScan(const std::string& t, const std::string& a = "") 
        : table_name(t), alias(a) {}
        
    std::string toJson() const override {
        std::string n = table_name + (alias.empty() ? "" : " AS " + alias);
        return "{\"type\": \"LogicalScan\", \"table\": \"" + n + "\"}";
    }
    
    std::string toSql() const override {
        return table_name + (alias.empty() ? "" : " " + alias);
    }
};

struct LogicalFilter : public LogicalNode {
    std::shared_ptr<ast::Expression> predicate;
    std::shared_ptr<LogicalNode> child;
    
    LogicalFilter(std::shared_ptr<ast::Expression> p, std::shared_ptr<LogicalNode> c)
        : predicate(p), child(c) {}
        
    std::string toJson() const override {
        return "{\"type\": \"LogicalFilter\", \"predicate\": \"" + predicate->toString() + "\", \"child\": " + child->toJson() + "}";
    }
    
    std::string toSql() const override {
        // Wraps the filtered child in a subquery to show the "pushdown"
        return "(SELECT * FROM " + child->toSql() + " WHERE " + predicate->toString() + ")";
    }
};

struct LogicalJoin : public LogicalNode {
    std::shared_ptr<LogicalNode> left;
    std::shared_ptr<LogicalNode> right;
    std::shared_ptr<ast::Expression> condition;
    std::string join_type;
    
    LogicalJoin(std::shared_ptr<LogicalNode> l, std::shared_ptr<LogicalNode> r, std::shared_ptr<ast::Expression> cond, const std::string& jt)
        : left(l), right(r), condition(cond), join_type(jt) {}
        
    std::string toJson() const override {
        std::string condStr = condition ? condition->toString() : "true";
        return "{\"type\": \"LogicalJoin\", \"join_type\": \"" + join_type + "\", \"condition\": \"" + condStr + "\", \"left\": " + left->toJson() + ", \"right\": " + right->toJson() + "}";
    }
    
    std::string toSql() const override {
        std::string condStr = condition ? condition->toString() : "1=1";
        return "(" + left->toSql() + " " + join_type + " JOIN " + right->toSql() + " ON " + condStr + ")";
    }
};

struct LogicalProject : public LogicalNode {
    std::vector<std::shared_ptr<ast::Expression>> select_list;
    std::shared_ptr<LogicalNode> child;
    
    LogicalProject(const std::vector<std::shared_ptr<ast::Expression>>& sl, std::shared_ptr<LogicalNode> c)
        : select_list(sl), child(c) {}
        
    std::string toJson() const override {
        std::string sl_str = "[";
        for (size_t i = 0; i < select_list.size(); ++i) {
            sl_str += "\"" + select_list[i]->toString() + "\"";
            if (i < select_list.size() - 1) sl_str += ", ";
        }
        sl_str += "]";
        return "{\"type\": \"LogicalProject\", \"columns\": " + sl_str + ", \"child\": " + child->toJson() + "}";
    }
    
    std::string toSql() const override {
        std::string sl_str = "";
        for (size_t i = 0; i < select_list.size(); ++i) {
            sl_str += select_list[i]->toString();
            if (i < select_list.size() - 1) sl_str += ", ";
        }
        return "SELECT " + sl_str + " \nFROM " + child->toSql();
    }
};

class PlanGenerator {
public:
    static std::shared_ptr<LogicalNode> createPlan(std::shared_ptr<ast::SelectStatement> ast) {
        if (!ast->from_source) return nullptr;
        auto root = buildSource(ast->from_source);
        if (ast->where_clause) {
            root = std::make_shared<LogicalFilter>(ast->where_clause, root);
        }
        if (!ast->select_list.empty()) {
            root = std::make_shared<LogicalProject>(ast->select_list, root);
        }
        return root;
    }
    
private:
    static std::shared_ptr<LogicalNode> buildSource(std::shared_ptr<ast::Source> source) {
        if (auto tableRef = std::dynamic_pointer_cast<ast::TableRef>(source)) {
            return std::make_shared<LogicalScan>(tableRef->table_name, tableRef->alias);
        } else if (auto joinNode = std::dynamic_pointer_cast<ast::JoinNode>(source)) {
            auto left = buildSource(joinNode->join_info.left);
            auto right = buildSource(joinNode->join_info.right);
            return std::make_shared<LogicalJoin>(left, right, joinNode->join_info.condition, joinNode->join_info.join_type);
        }
        return nullptr;
    }
};

} // namespace logical