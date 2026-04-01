#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct ColumnSet {
    std::string name;
    bool has_index;
    int num_distinct_values;
};

struct TableStats {
    std::string name;
    int num_rows;
    std::vector<ColumnSet> columns;
    
    bool hasColumn(const std::string& colName) const {
        for(const auto& c : columns) {
            if(c.name == colName) return true;
        }
        return false;
    }
    bool hasIndex(const std::string& colName) const {
         for(const auto& c : columns) {
            if(c.name == colName) return c.has_index;
        }
        return false;
    }
};

class Catalog {
public:
    Catalog() {
        // Mock data
        tables["users"] = {"users", 10000, {
            {"id", true, 10000},
            {"name", false, 9000},
            {"age", false, 80}
        }};
        
        tables["orders"] = {"orders", 50000, {
            {"id", true, 50000},
            {"user_id", true, 10000},
            {"amount", false, 500}
        }};
    }

    bool tableExists(const std::string& t) const {
        return tables.find(t) != tables.end();
    }
    
    TableStats getTable(const std::string& t) const {
        return tables.at(t);
    }

private:
    std::unordered_map<std::string, TableStats> tables;
};
