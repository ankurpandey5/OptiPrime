#pragma once
#include "logical_plan.hpp"
#include "catalog.hpp"

namespace physical {

struct PhysicalNode {
    int estimated_cost = 0;
    int estimated_rows = 0;
    virtual ~PhysicalNode() = default;
    virtual std::string toJson() const = 0;
};

struct SeqScan : public PhysicalNode {
    std::string table_name;
    SeqScan(const std::string& t, int rows) : table_name(t) {
        estimated_rows = rows;
        estimated_cost = rows * 1; 
    }
    std::string toJson() const override { 
        return "{\"type\": \"SeqScan\", \"table\": \"" + table_name + "\", \"cost\": " + std::to_string(estimated_cost) + ", \"rows\": " + std::to_string(estimated_rows) + "}"; 
    }
};

struct IndexScan : public PhysicalNode {
    std::string table_name;
    IndexScan(const std::string& t, int rows) : table_name(t) {
        estimated_rows = rows / 100; // Simulated strong selectivity
        estimated_cost = 10; 
    }
    std::string toJson() const override { 
        return "{\"type\": \"IndexScan\", \"table\": \"" + table_name + "\", \"cost\": " + std::to_string(estimated_cost) + ", \"rows\": " + std::to_string(estimated_rows) + "}"; 
    }
};

struct PhysicalFilter : public PhysicalNode {
    std::shared_ptr<PhysicalNode> child;
    PhysicalFilter(std::shared_ptr<PhysicalNode> c) : child(c) {
        estimated_rows = c->estimated_rows / 10;
        estimated_cost = c->estimated_cost + estimated_rows;
    }
    std::string toJson() const override { 
        return "{\"type\": \"PhysicalFilter\", \"cost\": " + std::to_string(estimated_cost) + ", \"child\": " + child->toJson() + "}"; 
    }
};

struct NestedLoopJoin : public PhysicalNode {
    std::shared_ptr<PhysicalNode> left;
    std::shared_ptr<PhysicalNode> right;
    NestedLoopJoin(std::shared_ptr<PhysicalNode> l, std::shared_ptr<PhysicalNode> r) : left(l), right(r) {
        estimated_rows = l->estimated_rows * r->estimated_rows;
        estimated_cost = l->estimated_cost + (l->estimated_rows * r->estimated_cost);
    }
    std::string toJson() const override { 
        return "{\"type\": \"NestedLoopJoin\", \"cost\": " + std::to_string(estimated_cost) + ", \"rows\": " + std::to_string(estimated_rows) + ", \"left\": " + left->toJson() + ", \"right\": " + right->toJson() + "}"; 
    }
};

struct HashJoin : public PhysicalNode {
    std::shared_ptr<PhysicalNode> left;
    std::shared_ptr<PhysicalNode> right;
    HashJoin(std::shared_ptr<PhysicalNode> l, std::shared_ptr<PhysicalNode> r) : left(l), right(r) {
        estimated_rows = (l->estimated_rows * r->estimated_rows) / 100; 
        estimated_cost = l->estimated_cost + r->estimated_cost + (l->estimated_rows * 2); 
    }
    std::string toJson() const override { 
        return "{\"type\": \"HashJoin\", \"cost\": " + std::to_string(estimated_cost) + ", \"rows\": " + std::to_string(estimated_rows) + ", \"left\": " + left->toJson() + ", \"right\": " + right->toJson() + "}"; 
    }
};

struct PhysicalProject : public PhysicalNode {
    std::shared_ptr<PhysicalNode> child;
    PhysicalProject(std::shared_ptr<PhysicalNode> c) : child(c) {
        estimated_rows = c->estimated_rows;
        estimated_cost = c->estimated_cost + estimated_rows; 
    }
    std::string toJson() const override { 
        return "{\"type\": \"PhysicalProject\", \"cost\": " + std::to_string(estimated_cost) + ", \"child\": " + child->toJson() + "}"; 
    }
};

class CostEstimator {
public:
    static std::shared_ptr<PhysicalNode> generate(std::shared_ptr<logical::LogicalNode> node, const Catalog& catalog) {
        if (!node) return nullptr;
        
        if (auto project = std::dynamic_pointer_cast<logical::LogicalProject>(node)) {
            return std::make_shared<PhysicalProject>(generate(project->child, catalog));
        }
        
        if (auto filter = std::dynamic_pointer_cast<logical::LogicalFilter>(node)) {
            if (auto scan = std::dynamic_pointer_cast<logical::LogicalScan>(filter->child)) {
                if (catalog.tableExists(scan->table_name)) {
                    auto stats = catalog.getTable(scan->table_name);
                    // Check if table has an indexed column, then select IndexScan instead of SeqScan!
                    if (stats.columns.size() > 0 && stats.columns[0].has_index) {
                        return std::make_shared<IndexScan>(scan->table_name, stats.num_rows);
                    }
                }
            }
            // Fallback for filter
            return std::make_shared<PhysicalFilter>(generate(filter->child, catalog));
        }
        
        if (auto scan = std::dynamic_pointer_cast<logical::LogicalScan>(node)) {
            int rows = 1000;
            if (catalog.tableExists(scan->table_name)) rows = catalog.getTable(scan->table_name).num_rows;
            return std::make_shared<SeqScan>(scan->table_name, rows);
        }
        
        if (auto join = std::dynamic_pointer_cast<logical::LogicalJoin>(node)) {
            auto left = generate(join->left, catalog);
            auto right = generate(join->right, catalog);
            
            auto nlj = std::make_shared<NestedLoopJoin>(left, right);
            auto hj = std::make_shared<HashJoin>(left, right);
            
            // MAGIC: Select the cheapest physical join operator
            if (hj->estimated_cost < nlj->estimated_cost) {
                return hj;
            }
            return nlj;
        }
        
        return nullptr;
    }
};
} // namespace physical
