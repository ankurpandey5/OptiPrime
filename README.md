# 🚀 Mini Query Optimizer

An educational prototype demonstrating how a declarative SQL query transforms into a physical execution plan. Built in Modern C++ using an embedded Single-Page Web Application for visualizing the abstract syntax trees.

## 🌟 Features
- **Custom SQL Parser**: A recursive descent parser that natively tokenizes and builds an AST for a minimal subset of SQL (`SELECT`, `JOIN`, `WHERE`).
- **Semantic Analyzer**: Validates queries against a mocked `Catalog` registry containing mock table sizes and mock index definitions.
- **Relational Algebra**: Generates a Naïve Logical Plan utilizing `LogicalScan`, `LogicalJoin`, and `LogicalFilter`.
- **Rule-Based Optimizer**: Applies standard heuristic optimizer rules (like **Predicate Pushdown**).
- **Cost Estimator**: Uses table cardinality to pick the cheapest physical operations (e.g., swapping a `NestedLoopJoin` for a `HashJoin` on large datasets, or selecting an `IndexScan` over a `SeqScan`).
- **Web UI Visualization**: Uses an integrated C++ HTTP Server connecting to a gorgeous Dark-Mode Glassmorphism frontend running Mermaid.js side-by-side tree comparisons.

## 🏗️ Architecture Pipeline
1. `parser.hpp`: Raw SQL string -> Abstract Syntax Tree (AST).
2. `logical_plan.hpp`: AST -> Relational Algebra Tree.
3. `optimizer.hpp`: Relational Algebra Tree -> Optimized Logical Tree (Pushdown Filters).
4. `physical_plan.hpp`: Optimized Logical Tree -> Physical Plan Trees (Costed).
5. Frontend (`script.js`): Maps Physical JSON output to Mermaid.js SVG trees.

## 🛠️ Build & Run Instructions

This codebase is pure C++17 with single-header dependencies (`cpp-httplib` and `nlohmann-json`), making it extremely portable.

### System Requirements
- CMake 3.14+
- A modern C++17 compiler (GCC, Clang, or MSVC)

### Building
Open your terminal in the `mini_optimizer` directory:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
*(If on Windows using MSVC, run this from the Native Tools Command Prompt)*

### Running
Simply execute the built binary:
```bash
# On Linux/Mac:
./build/mini_optimizer

# On Windows:
.\build\Debug\mini_optimizer.exe
```

Once running, navigate to [http://localhost:8080](http://localhost:8080) in your web browser to enter queries and see the visualization!

## 🧪 Example Supported Queries

Try entering these into the Web UI:

**Simple Filters (SeqScan):**
```sql
SELECT name FROM users WHERE age > 30
```

**Index Selection & Pushdown:**
```sql
-- 'id' has a mocked index in the catalog, triggering an IndexScan!
SELECT id, name FROM users WHERE id = 123 
```

**Join Reordering & HashJoins:**
```sql
SELECT users.id FROM users JOIN orders ON users.id = orders.user_id WHERE users.age = 45
```
