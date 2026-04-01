#pragma once
#include "logical_plan.hpp"

namespace optimizer {

class RuleBasedOptimizer {
public:
    static std::shared_ptr<logical::LogicalNode> optimize(std::shared_ptr<logical::LogicalNode> root) {
        // Apply rules sequentially
        return pushdownFilters(root);
    }
    
private:
    static std::shared_ptr<logical::LogicalNode> pushdownFilters(std::shared_ptr<logical::LogicalNode> node) {
        if (!node) return nullptr;
        
        if (auto project = std::dynamic_pointer_cast<logical::LogicalProject>(node)) {
            project->child = pushdownFilters(project->child);
            return project;
        }
        
        if (auto filter = std::dynamic_pointer_cast<logical::LogicalFilter>(node)) {
            // Predicate Pushdown logic
            // If the child is a Join, push the filter down into the child
            if (auto join = std::dynamic_pointer_cast<logical::LogicalJoin>(filter->child)) {
                
                // For this educational prototype, we blindly push it down to the left child
                // A real optimizer analyzes AST to find which table the predicate belongs to
                
                auto newFilter = std::make_shared<logical::LogicalFilter>(filter->predicate, join->left);
                join->left = pushdownFilters(newFilter); 
                join->right = pushdownFilters(join->right);
                
                // The filter has been successfully pushed down! Return the join instead of the filter
                return join; 
            }
            
            filter->child = pushdownFilters(filter->child);
            return filter;
        }
        
        if (auto join = std::dynamic_pointer_cast<logical::LogicalJoin>(node)) {
            join->left = pushdownFilters(join->left);
            join->right = pushdownFilters(join->right);
            return join;
        }
        
        return node;
    }
};

} // namespace optimizer
