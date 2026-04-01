#include "parser.hpp"

const SqlToken& Parser::current() const {
    if (pos >= tokens.size()) return tokens.back();
    return tokens[pos];
}

const SqlToken& Parser::consume() {
    if (pos >= tokens.size()) return tokens.back();
    return tokens[pos++];
}

bool Parser::match(SqlTokenType t, const std::string& val) {
    if (current().type == t && (val.empty() || current().value == val)) {
        pos++;
        return true;
    }
    return false;
}

std::shared_ptr<ast::SelectStatement> Parser::parse() {
    auto stmt = std::make_shared<ast::SelectStatement>();

    if (!match(SqlTokenType::Keyword, "SELECT")) {
        throw std::runtime_error("Expected SELECT keyword");
    }

    do {
        stmt->select_list.push_back(parseExpression());
    } while (match(SqlTokenType::Symbol, ","));

    if (match(SqlTokenType::Keyword, "FROM")) {
        stmt->from_source = parseSource();
        
        while (current().value == "JOIN" || current().value == "INNER" || 
               current().value == "LEFT" || current().value == "RIGHT") {
            stmt->from_source = parseJoin(stmt->from_source);
        }
    }

    if (match(SqlTokenType::Keyword, "WHERE")) {
        stmt->where_clause = parseExpression();
    }

    return stmt;
}

std::shared_ptr<ast::Expression> Parser::parseExpression() {
    auto left = parsePrimary();

    while (current().type == SqlTokenType::Symbol || 
           (current().type == SqlTokenType::Keyword && 
            (current().value == "AND" || current().value == "OR"))) {
        
        std::string op = consume().value;
        auto right = parsePrimary();
        
        left = std::make_shared<ast::BinaryExpr>(op, left, right);
    }

    return left;
}

std::shared_ptr<ast::Expression> Parser::parsePrimary() {
    if (current().type == SqlTokenType::Number) {
        return std::make_shared<ast::Literal>(consume().value, true);
    }
    
    if (current().type == SqlTokenType::String) {
        return std::make_shared<ast::Literal>(consume().value, false);
    }

    if (current().type == SqlTokenType::Identifier) {
        std::string first = consume().value;
        if (match(SqlTokenType::Symbol, ".")) {
            if (current().type != SqlTokenType::Identifier) {
                throw std::runtime_error("Expected column name after '.'");
            }
            std::string second = consume().value;
            return std::make_shared<ast::ColumnRef>(first, second);
        }
        return std::make_shared<ast::ColumnRef>("", first);
    }

    throw std::runtime_error("Unexpected token in expression: " + current().value);
}

std::shared_ptr<ast::Source> Parser::parseSource() {
    if (current().type != SqlTokenType::Identifier) {
        throw std::runtime_error("Expected table name but got: " + current().value);
    }
    
    std::string tableName = consume().value;
    std::string alias = "";

    if (match(SqlTokenType::Keyword, "AS")) {
        if (current().type != SqlTokenType::Identifier) {
            throw std::runtime_error("Expected alias after AS");
        }
        alias = consume().value;
    } else if (current().type == SqlTokenType::Identifier) {
        alias = consume().value;
    }

    return std::make_shared<ast::TableRef>(tableName, alias);
}

std::shared_ptr<ast::JoinNode> Parser::parseJoin(std::shared_ptr<ast::Source> left) {
    ast::JoinData jd;
    jd.left = left;
    jd.join_type = "INNER"; 

    if (match(SqlTokenType::Keyword, "LEFT") || match(SqlTokenType::Keyword, "RIGHT")) {
        jd.join_type = tokens[pos - 1].value;
        match(SqlTokenType::Keyword, "OUTER"); 
    } else if (match(SqlTokenType::Keyword, "INNER")) {
    }

    if (!match(SqlTokenType::Keyword, "JOIN")) {
        throw std::runtime_error("Expected JOIN keyword");
    }

    jd.right = parseSource();

    if (match(SqlTokenType::Keyword, "ON")) {
        jd.condition = parseExpression();
    }

    return std::make_shared<ast::JoinNode>(jd);
}