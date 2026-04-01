#pragma once

#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

// Renamed to avoid Windows header collisions!
enum class SqlTokenType {
    Keyword,
    Identifier,
    Number,
    String,
    Symbol,
    EndOfFile
};

struct SqlToken {
    SqlTokenType type;
    std::string value;
};

class Lexer {
public:
    explicit Lexer(const std::string& sql) : input(sql), pos(0) {}

    std::vector<SqlToken> tokenize() {
        std::vector<SqlToken> tokens;
        while (pos < input.length()) {
            char c = input[pos];
            if (std::isspace(c)) {
                pos++;
            } else if (std::isalpha(c) || c == '_') {
                tokens.push_back(readIdentifierOrKeyword());
            } else if (std::isdigit(c)) {
                tokens.push_back(readNumber());
            } else if (c == '\'') {
                tokens.push_back(readString());
            } else {
                tokens.push_back(readSymbol());
            }
        }
        tokens.push_back({SqlTokenType::EndOfFile, ""});
        return tokens;
    }

private:
    std::string input;
    size_t pos;

    bool isKeyword(const std::string& str) {
        std::string upper;
        for (char c : str) upper += std::toupper(c);
        return upper == "SELECT" || upper == "FROM" || upper == "WHERE" ||
               upper == "JOIN" || upper == "ON" || upper == "INNER" || 
               upper == "LEFT" || upper == "RIGHT" || upper == "AS" || 
               upper == "AND" || upper == "OR";
    }

    SqlToken readIdentifierOrKeyword() {
        std::string val;
        while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '_')) {
            val += input[pos++];
        }
        if (isKeyword(val)) {
            for (char& c : val) c = std::toupper(c);
            return {SqlTokenType::Keyword, val};
        }
        return {SqlTokenType::Identifier, val};
    }

    SqlToken readNumber() {
        std::string val;
        while (pos < input.length() && std::isdigit(input[pos])) {
            val += input[pos++];
        }
        return {SqlTokenType::Number, val};
    }

    SqlToken readString() {
        std::string val;
        pos++; 
        while (pos < input.length() && input[pos] != '\'') {
            val += input[pos++];
        }
        if (pos < input.length()) pos++; 
        return {SqlTokenType::String, val};
    }

    SqlToken readSymbol() {
        char c = input[pos++];
        std::string val(1, c);
        
        if (pos < input.length()) {
            char next = input[pos];
            if ((c == '>' && next == '=') || 
                (c == '<' && next == '=') || 
                (c == '!' && next == '=')) {
                val += next;
                pos++;
            }
        }
        return {SqlTokenType::Symbol, val};
    }
};