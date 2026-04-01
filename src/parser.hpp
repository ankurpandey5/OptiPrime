#pragma once

#include "lexer.hpp"
#include "ast.hpp"
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>

class Parser {
public:
    explicit Parser(std::vector<SqlToken> t) : tokens(std::move(t)), pos(0) {}

    std::shared_ptr<ast::SelectStatement> parse();

private:
    std::vector<SqlToken> tokens;
    size_t pos;

    const SqlToken& current() const;
    const SqlToken& consume();
    bool match(SqlTokenType t, const std::string& val = "");

    std::shared_ptr<ast::Expression> parseExpression();
    std::shared_ptr<ast::Expression> parsePrimary();
    std::shared_ptr<ast::Source> parseSource();
    std::shared_ptr<ast::JoinNode> parseJoin(std::shared_ptr<ast::Source> left);
};