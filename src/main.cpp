#include <iostream>
#include <string>
#include <exception>

#include "httplib.h"
#include "json.hpp"

#include "parser.hpp"
#include "catalog.hpp"
#include "logical_plan.hpp"
#include "optimizer.hpp"
#include "physical_plan.hpp"

using json = nlohmann::json;

int main() {
    httplib::Server svr;
    Catalog catalog;

    // Serve Static Files for our beautiful UI
    auto ret = svr.set_mount_point("/", "./public");
    if (!ret) {
        std::cerr << "Warning: 'public' directory not found. Please run from e:/OPTIMUSPRIME/mini_optimizer/\n";
    }

    // API Endpoint for the Query Optimizer
    svr.Post("/api/optimize", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto req_json = json::parse(req.body);
            std::string query = req_json["query"];

            std::cout << "[Processing Query]: " << query << "\n";

            // 1. Parsing and AST Generation
            Lexer lexer(query);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto ast = parser.parse();

            // 2. Logical Relational Algebra (Naive Plan)
            auto logical_tree = logical::PlanGenerator::createPlan(ast);
            if (!logical_tree) {
                throw std::runtime_error("Empty or invalid logical plan generated.");
            }

            // 3. Rule-Based Optimization (Predicate Pushdown)
            auto optimized_tree = optimizer::RuleBasedOptimizer::optimize(logical_tree);

            // Print the reverse-translated optimized SQL to the terminal
            std::cout << "\n[REWRITTEN OPTIMIZED SQL]:\n";
            std::cout << optimized_tree->toSql() << "\n\n";

            // 4. Cost Estimation & Physical Plan
            auto physical_tree = physical::CostEstimator::generate(optimized_tree, catalog);

            // Format JSON Response
            json res_json;
            res_json["logical_plan"] = json::parse(logical_tree->toJson());
            res_json["physical_plan"] = json::parse(physical_tree->toJson());
            res_json["optimized_sql"] = optimized_tree->toSql(); // Send rewritten SQL to frontend

            res.set_content(res_json.dump(), "application/json");

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            json err_json;
            err_json["error"] = e.what();
            res.status = 400;
            res.set_content(err_json.dump(), "application/json");
        }
    });

    std::cout << "=======================================\n";
    std::cout << "    Mini Query Optimizer Web Engine    \n";
    std::cout << "=======================================\n";
    std::cout << "Starting server on http://localhost:8081\n";
    
    if (!svr.listen("localhost", 8081)) {
        std::cerr << "\n[ERROR] Failed to start server! Port 8081 might be in use.\n";
    }
    
    return 0;
}