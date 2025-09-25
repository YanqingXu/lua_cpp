/**
 * @file parser.cpp
 * @brief Lua语法分析器实现
 * @description 实现Lua 5.1.5的完整语法分析，构建抽象语法树(AST)
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "parser.h"
#include <sstream>
#include <algorithm>

namespace lua_cpp {

/* ========================================================================== */
/* Parser使用lexer_errors.h中已定义的错误类型 */
/* ========================================================================== */

/* ========================================================================== */
/* Parser类实现 */
/* ========================================================================== */

Parser::Parser(std::unique_ptr<Lexer> lexer, const ParserConfig& config)
    : lexer_(std::move(lexer)), config_(config), state_(ParserState::Ready),
      recovery_strategy_(RecoveryStrategy::Synchronize),
      current_token_(Token::CreateEndOfSource(1, 1)),
      peek_token_(Token::CreateEndOfSource(1, 1)),
      has_peek_token_(false),
      error_count_(0), recursion_depth_(0), expression_depth_(0) {
    
    if (!lexer_) {
        throw std::invalid_argument("Lexer cannot be null");
    }
    
    // 初始化增强错误恢复系统
    if (config_.use_enhanced_error_recovery) {
        error_collector_ = std::make_unique<ErrorCollector>();
        recovery_engine_ = std::make_unique<ErrorRecoveryEngine>();
        if (config_.generate_error_suggestions) {
            suggestion_generator_ = std::make_unique<ErrorSuggestionGenerator>();
        }
        error_formatter_ = std::make_unique<Lua51ErrorFormatter>();
    }
    
    InitializeState();
    
    // 预读取前两个token
    Advance();
}

bool Parser::IsAtEnd() const {
    return current_token_.GetType() == TokenType::EndOfSource;
}

const Token& Parser::GetCurrentToken() const {
    return current_token_;
}

const Token& Parser::PeekToken() const {
    if (!has_peek_token_) {
        // 懒加载peek token
        const_cast<Parser*>(this)->peek_token_ = lexer_->PeekToken();
        const_cast<Parser*>(this)->has_peek_token_ = true;
    }
    return peek_token_;
}

SourcePosition Parser::GetCurrentPosition() const {
    return SourcePosition{current_token_.GetLine(), current_token_.GetColumn()};
}

/* ========================================================================== */
/* Token操作 */
/* ========================================================================== */

void Parser::Advance() {
    if (has_peek_token_) {
        current_token_ = std::move(peek_token_);
        peek_token_ = Token::CreateEndOfSource(1, 1);
        has_peek_token_ = false;
    } else {
        current_token_ = lexer_->NextToken();
    }
}

bool Parser::Match(TokenType type) {
    if (current_token_.GetType() == type) {
        Advance();
        return true;
    }
    return false;
}

Token Parser::Consume(TokenType expected) {
    if (current_token_.GetType() != expected) {
        throw UnexpectedTokenError(expected, current_token_.GetType(), GetCurrentPosition());
    }
    Token token = current_token_;
    Advance();
    return token;
}

Token Parser::Consume(TokenType expected, const std::string& message) {
    if (current_token_.GetType() != expected) {
        throw SyntaxError(message, GetCurrentPosition());
    }
    Token token = current_token_;
    Advance();
    return token;
}

bool Parser::Check(TokenType type) const {
    return current_token_.GetType() == type;
}

bool Parser::CheckAny(const std::vector<TokenType>& types) const {
    return std::find(types.begin(), types.end(), current_token_.GetType()) != types.end();
}

/* ========================================================================== */
/* 解析入口点 */
/* ========================================================================== */

std::unique_ptr<Program> Parser::ParseProgram() {
    state_ = ParserState::Parsing;
    
    try {
        auto program = std::make_unique<Program>(GetCurrentPosition());
        
        // 解析顶层语句块
        while (!IsAtEnd()) {
            auto statement = ParseStatement();
            if (statement) {
                program->AddStatement(std::move(statement));
            }
        }
        
        state_ = ParserState::Completed;
        return program;
        
    } catch (const SyntaxError& e) {
        state_ = ParserState::Error;
        if (config_.recover_from_errors) {
            // TODO: 实现错误恢复
            throw;
        } else {
            throw;
        }
    }
}

std::unique_ptr<Statement> Parser::ParseStatement() {
    CheckRecursionDepth();
    
    SourcePosition start_pos = GetCurrentPosition();
    
    // 跳过分号
    while (Match(TokenType::Semicolon)) {
        // 空语句，继续
    }
    
    if (IsAtEnd()) {
        return nullptr;
    }
    
    // 根据当前token类型选择解析方法
    switch (current_token_.GetType()) {
        case TokenType::Local:
            return ParseLocalStatement();
            
        case TokenType::If:
            return ParseIfStatement();
            
        case TokenType::While:
            return ParseWhileStatement();
            
        case TokenType::Repeat:
            return ParseRepeatStatement();
            
        case TokenType::For:
            return ParseForStatement();
            
        case TokenType::Function:
            return ParseFunctionDefinition();
            
        case TokenType::Do:
            return ParseDoStatement();
            
        case TokenType::Break:
            return ParseBreakStatement();
            
        case TokenType::Return:
            return ParseReturnStatement();
            
        default:
            // 尝试解析赋值语句或表达式语句
            return ParseAssignmentOrExpressionStatement();
    }
}

std::unique_ptr<Expression> Parser::ParseExpression() {
    return ParseExpression(Precedence::Assignment);
}

std::unique_ptr<Expression> Parser::ParseExpression(Precedence min_precedence) {
    CheckExpressionDepth();
    
    expression_depth_++;
    
    try {
        auto left = ParsePrimaryExpression();
        
        while (true) {
            auto current_precedence = GetBinaryOperatorPrecedence(current_token_.GetType());
            if (current_precedence < min_precedence) {
                break;
            }
            
            auto operator_type = current_token_.GetType();
            Advance();
            
            auto right_precedence = current_precedence;
            if (IsRightAssociative(operator_type)) {
                right_precedence = Precedence(static_cast<int>(current_precedence) - 1);
            } else {
                right_precedence = Precedence(static_cast<int>(current_precedence) + 1);
            }
            
            auto right = ParseExpression(right_precedence);
            
            // 创建二元表达式
            BinaryOperator op = TokenTypeToBinaryOperator(operator_type);
            left = std::make_unique<BinaryExpression>(op, std::move(left), std::move(right), left->GetPosition());
        }
        
        expression_depth_--;
        return left;
        
    } catch (...) {
        expression_depth_--;
        throw;
    }
}

/* ========================================================================== */
/* 辅助方法 */
/* ========================================================================== */

void Parser::CheckRecursionDepth() {
    if (recursion_depth_ >= config_.max_recursion_depth) {
        throw SyntaxError("Maximum recursion depth exceeded", GetCurrentPosition());
    }
    recursion_depth_++;
}

void Parser::CheckExpressionDepth() {
    if (expression_depth_ >= config_.max_expression_depth) {
        throw SyntaxError("Maximum expression depth exceeded", GetCurrentPosition());
    }
}

Precedence Parser::GetBinaryOperatorPrecedence(TokenType type) const {
    switch (type) {
        case TokenType::Or:
            return Precedence::Or;
        case TokenType::And:
            return Precedence::And;
        case TokenType::Less:
        case TokenType::Greater:
        case TokenType::LessEqual:
        case TokenType::GreaterEqual:
        case TokenType::Equal:
        case TokenType::NotEqual:
            return Precedence::Comparison;
        case TokenType::Concat:
            return Precedence::Concatenate;
        case TokenType::Plus:
        case TokenType::Minus:
            return Precedence::Term;
        case TokenType::Multiply:
        case TokenType::Divide:
        case TokenType::Modulo:
            return Precedence::Factor;
        case TokenType::Power:
            return Precedence::Power;
        default:
            return Precedence::None;
    }
}

bool Parser::IsRightAssociative(TokenType type) const {
    return type == TokenType::Power || type == TokenType::Concat;
}

BinaryOperator Parser::TokenTypeToBinaryOperator(TokenType type) const {
    switch (type) {
        case TokenType::Plus: return BinaryOperator::Add;
        case TokenType::Minus: return BinaryOperator::Subtract;
        case TokenType::Multiply: return BinaryOperator::Multiply;
        case TokenType::Divide: return BinaryOperator::Divide;
        case TokenType::Modulo: return BinaryOperator::Modulo;
        case TokenType::Power: return BinaryOperator::Power;
        case TokenType::Equal: return BinaryOperator::Equal;
        case TokenType::NotEqual: return BinaryOperator::NotEqual;
        case TokenType::Less: return BinaryOperator::Less;
        case TokenType::LessEqual: return BinaryOperator::LessEqual;
        case TokenType::Greater: return BinaryOperator::Greater;
        case TokenType::GreaterEqual: return BinaryOperator::GreaterEqual;
        case TokenType::And: return BinaryOperator::And;
        case TokenType::Or: return BinaryOperator::Or;
        case TokenType::Concat: return BinaryOperator::Concat;
        default:
            throw SyntaxError("Invalid binary operator", GetCurrentPosition());
    }
}

UnaryOperator Parser::TokenTypeToUnaryOperator(TokenType type) const {
    switch (type) {
        case TokenType::Minus: return UnaryOperator::Minus;
        case TokenType::Not: return UnaryOperator::Not;
        case TokenType::Length: return UnaryOperator::Length;
        default:
            throw SyntaxError("Invalid unary operator", GetCurrentPosition());
    }
}

/* ========================================================================== */
/* 语句解析实现 */
/* ========================================================================== */

std::unique_ptr<Statement> Parser::ParseLocalStatement() {
    Consume(TokenType::Local);
    
    if (Check(TokenType::Function)) {
        return ParseLocalFunctionDefinition();
    } else {
        return ParseLocalDeclaration();
    }
}

std::unique_ptr<LocalDeclaration> Parser::ParseLocalDeclaration() {
    SourcePosition start_pos = GetCurrentPosition();
    
    // 解析变量列表
    std::vector<std::string> variables;
    variables.push_back(GetCurrentToken().GetValue());
    Consume(TokenType::Identifier);
    
    while (Match(TokenType::Comma)) {
        variables.push_back(GetCurrentToken().GetValue());
        Consume(TokenType::Identifier);
    }
    
    // 解析初始化表达式列表（可选）
    std::vector<std::unique_ptr<Expression>> values;
    if (Match(TokenType::Assign)) {
        values.push_back(ParseExpression());
        
        while (Match(TokenType::Comma)) {
            values.push_back(ParseExpression());
        }
    }
    
    return std::make_unique<LocalDeclaration>(std::move(variables), std::move(values), start_pos);
}

std::unique_ptr<LocalFunctionDefinition> Parser::ParseLocalFunctionDefinition() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::Function);
    
    std::string name = GetCurrentToken().GetValue();
    Consume(TokenType::Identifier);
    
    // 解析参数列表
    bool is_vararg = false;
    auto parameters = ParseParameterList(is_vararg);
    
    // 解析函数体
    auto body = ParseBlock();
    Consume(TokenType::End);
    
    return std::make_unique<LocalFunctionDefinition>(name, std::move(parameters), 
                                                     std::move(body), is_vararg, start_pos);
}

std::unique_ptr<FunctionDefinition> Parser::ParseFunctionDefinition() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::Function);
    
    // 解析函数名（可能是复杂的表达式）
    auto name = ParseExpression();
    
    // 解析参数列表
    bool is_vararg = false;
    auto parameters = ParseParameterList(is_vararg);
    
    // 解析函数体
    auto body = ParseBlock();
    Consume(TokenType::End);
    
    return std::make_unique<FunctionDefinition>(std::move(name), std::move(parameters),
                                                std::move(body), is_vararg, start_pos);
}

std::unique_ptr<IfStatement> Parser::ParseIfStatement() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::If);
    auto condition = ParseExpression();
    Consume(TokenType::Then);
    auto then_block = ParseBlock();
    
    // 解析elseif子句
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<BlockNode>>> elseif_clauses;
    while (Check(TokenType::Elseif)) {
        Advance(); // consume 'elseif'
        auto elseif_condition = ParseExpression();
        Consume(TokenType::Then);
        auto elseif_block = ParseBlock();
        elseif_clauses.emplace_back(std::move(elseif_condition), std::move(elseif_block));
    }
    
    // 解析else子句（可选）
    std::unique_ptr<BlockNode> else_block = nullptr;
    if (Match(TokenType::Else)) {
        else_block = ParseBlock();
    }
    
    Consume(TokenType::End);
    
    return std::make_unique<IfStatement>(std::move(condition), std::move(then_block),
                                         std::move(elseif_clauses), std::move(else_block), start_pos);
}

std::unique_ptr<WhileStatement> Parser::ParseWhileStatement() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::While);
    auto condition = ParseExpression();
    Consume(TokenType::Do);
    auto body = ParseBlock();
    Consume(TokenType::End);
    
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body), start_pos);
}

std::unique_ptr<RepeatStatement> Parser::ParseRepeatStatement() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::Repeat);
    auto body = ParseBlock();
    Consume(TokenType::Until);
    auto condition = ParseExpression();
    
    return std::make_unique<RepeatStatement>(std::move(body), std::move(condition), start_pos);
}

std::unique_ptr<Statement> Parser::ParseForStatement() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::For);
    
    // 第一个变量名
    std::string first_var = GetCurrentToken().GetValue();
    Consume(TokenType::Identifier);
    
    if (Match(TokenType::Assign)) {
        // 数值for循环: for var = start, end [, step] do ... end
        return ParseNumericForStatement(first_var);
    } else {
        // 泛型for循环: for var1, var2, ... in expr1, expr2, ... do ... end
        std::vector<std::string> variables = {first_var};
        
        while (Match(TokenType::Comma)) {
            variables.push_back(GetCurrentToken().GetValue());
            Consume(TokenType::Identifier);
        }
        
        return ParseGenericForStatement(std::move(variables));
    }
}

std::unique_ptr<NumericForStatement> Parser::ParseNumericForStatement(const std::string& variable) {
    SourcePosition start_pos = GetCurrentPosition();
    
    auto start = ParseExpression();
    Consume(TokenType::Comma);
    auto end = ParseExpression();
    
    std::unique_ptr<Expression> step = nullptr;
    if (Match(TokenType::Comma)) {
        step = ParseExpression();
    }
    
    Consume(TokenType::Do);
    auto body = ParseBlock();
    Consume(TokenType::End);
    
    return std::make_unique<NumericForStatement>(variable, std::move(start), std::move(end),
                                                 std::move(step), std::move(body), start_pos);
}

std::unique_ptr<GenericForStatement> Parser::ParseGenericForStatement(std::vector<std::string> variables) {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::In);
    
    std::vector<std::unique_ptr<Expression>> expressions;
    expressions.push_back(ParseExpression());
    
    while (Match(TokenType::Comma)) {
        expressions.push_back(ParseExpression());
    }
    
    Consume(TokenType::Do);
    auto body = ParseBlock();
    Consume(TokenType::End);
    
    return std::make_unique<GenericForStatement>(std::move(variables), std::move(expressions),
                                                 std::move(body), start_pos);
}

std::unique_ptr<DoStatement> Parser::ParseDoStatement() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::Do);
    auto body = ParseBlock();
    Consume(TokenType::End);
    
    return std::make_unique<DoStatement>(std::move(body), start_pos);
}

std::unique_ptr<BreakStatement> Parser::ParseBreakStatement() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::Break);
    
    return std::make_unique<BreakStatement>(start_pos);
}

std::unique_ptr<ReturnStatement> Parser::ParseReturnStatement() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::Return);
    
    std::vector<std::unique_ptr<Expression>> values;
    
    // return语句后可能没有表达式
    if (!Check(TokenType::End) && !Check(TokenType::Else) && !Check(TokenType::Elseif) &&
        !Check(TokenType::Until) && !Check(TokenType::EndOfSource) && !Check(TokenType::Semicolon)) {
        
        values.push_back(ParseExpression());
        
        while (Match(TokenType::Comma)) {
            values.push_back(ParseExpression());
        }
    }
    
    return std::make_unique<ReturnStatement>(std::move(values), start_pos);
}

std::unique_ptr<Statement> Parser::ParseAssignmentOrExpressionStatement() {
    SourcePosition start_pos = GetCurrentPosition();
    
    // 先解析第一个表达式
    auto first_expr = ParseExpression();
    
    // 检查是否是赋值语句
    if (Check(TokenType::Assign) || Check(TokenType::Comma)) {
        std::vector<std::unique_ptr<Expression>> targets;
        targets.push_back(std::move(first_expr));
        
        // 如果有逗号，继续解析目标表达式
        while (Match(TokenType::Comma)) {
            targets.push_back(ParseExpression());
        }
        
        // 现在应该有赋值符号
        Consume(TokenType::Assign);
        
        // 解析值表达式
        std::vector<std::unique_ptr<Expression>> values;
        values.push_back(ParseExpression());
        
        while (Match(TokenType::Comma)) {
            values.push_back(ParseExpression());
        }
        
        return std::make_unique<AssignmentStatement>(std::move(targets), std::move(values), start_pos);
    } else {
        // 表达式语句
        return std::make_unique<ExpressionStatement>(std::move(first_expr), start_pos);
    }
}

/* ========================================================================== */
/* 表达式解析实现 */
/* ========================================================================== */

std::unique_ptr<Expression> Parser::ParsePrimaryExpression() {
    SourcePosition start_pos = GetCurrentPosition();
    
    switch (current_token_.GetType()) {
        case TokenType::Nil:
            Advance();
            return std::make_unique<NilLiteral>(start_pos);
            
        case TokenType::True:
            Advance();
            return std::make_unique<BooleanLiteral>(true, start_pos);
            
        case TokenType::False:
            Advance();
            return std::make_unique<BooleanLiteral>(false, start_pos);
            
        case TokenType::Number: {
            double value = std::stod(current_token_.GetValue());
            Advance();
            return std::make_unique<NumberLiteral>(value, start_pos);
        }
        
        case TokenType::String: {
            std::string value = current_token_.GetValue();
            Advance();
            return std::make_unique<StringLiteral>(value, start_pos);
        }
        
        case TokenType::Vararg:
            Advance();
            return std::make_unique<VarargLiteral>(start_pos);
            
        case TokenType::Identifier: {
            std::string name = current_token_.GetValue();
            Advance();
            auto identifier = std::make_unique<Identifier>(name, start_pos);
            return ParsePostfixExpression(std::move(identifier));
        }
        
        case TokenType::LeftParen: {
            Advance(); // consume '('
            auto expr = ParseExpression();
            Consume(TokenType::RightParen);
            return ParsePostfixExpression(std::move(expr));
        }
        
        case TokenType::LeftBrace:
            return ParsePostfixExpression(ParseTableConstructor());
            
        case TokenType::Function:
            return ParsePostfixExpression(ParseFunctionExpression());
            
        case TokenType::Minus:
        case TokenType::Not:
        case TokenType::Length:
            return ParseUnaryExpression();
            
        default:
            throw UnexpectedTokenError("expression", current_token_.GetType(), start_pos);
    }
}

std::unique_ptr<Expression> Parser::ParsePostfixExpression(std::unique_ptr<Expression> expr) {
    while (true) {
        switch (current_token_.GetType()) {
            case TokenType::LeftParen:
                expr = ParseCallExpression(std::move(expr));
                break;
                
            case TokenType::LeftBracket:
                expr = ParseIndexExpression(std::move(expr));
                break;
                
            case TokenType::Dot:
                expr = ParseMemberExpression(std::move(expr));
                break;
                
            case TokenType::Colon:
                expr = ParseMethodCallExpression(std::move(expr));
                break;
                
            case TokenType::String:
            case TokenType::LeftBrace:
                // 字符串或表可以作为函数调用的参数
                expr = ParseCallExpression(std::move(expr));
                break;
                
            default:
                return expr;
        }
    }
}

std::unique_ptr<Expression> Parser::ParseUnaryExpression() {
    SourcePosition start_pos = GetCurrentPosition();
    
    UnaryOperator op = TokenTypeToUnaryOperator(current_token_.GetType());
    Advance();
    
    auto operand = ParseExpression(Precedence::Unary);
    
    return std::make_unique<UnaryExpression>(op, std::move(operand), start_pos);
}

std::unique_ptr<Expression> Parser::ParseCallExpression(std::unique_ptr<Expression> function) {
    SourcePosition start_pos = function->GetPosition();
    
    std::vector<std::unique_ptr<Expression>> arguments;
    
    if (Match(TokenType::LeftParen)) {
        // 标准函数调用 func(arg1, arg2, ...)
        if (!Check(TokenType::RightParen)) {
            arguments.push_back(ParseExpression());
            
            while (Match(TokenType::Comma)) {
                arguments.push_back(ParseExpression());
            }
        }
        Consume(TokenType::RightParen);
    } else if (Check(TokenType::String)) {
        // 字符串作为单个参数 func"string"
        std::string value = current_token_.GetValue();
        Advance();
        arguments.push_back(std::make_unique<StringLiteral>(value, GetCurrentPosition()));
    } else if (Check(TokenType::LeftBrace)) {
        // 表作为单个参数 func{table}
        arguments.push_back(ParseTableConstructor());
    }
    
    return std::make_unique<CallExpression>(std::move(function), std::move(arguments), start_pos);
}

std::unique_ptr<Expression> Parser::ParseIndexExpression(std::unique_ptr<Expression> object) {
    SourcePosition start_pos = object->GetPosition();
    
    Consume(TokenType::LeftBracket);
    auto index = ParseExpression();
    Consume(TokenType::RightBracket);
    
    return std::make_unique<IndexExpression>(std::move(object), std::move(index), start_pos);
}

std::unique_ptr<Expression> Parser::ParseMemberExpression(std::unique_ptr<Expression> object) {
    SourcePosition start_pos = object->GetPosition();
    
    Consume(TokenType::Dot);
    std::string property = current_token_.GetValue();
    Consume(TokenType::Identifier);
    
    return std::make_unique<MemberExpression>(std::move(object), property, start_pos);
}

std::unique_ptr<Expression> Parser::ParseMethodCallExpression(std::unique_ptr<Expression> object) {
    SourcePosition start_pos = object->GetPosition();
    
    Consume(TokenType::Colon);
    std::string method = current_token_.GetValue();
    Consume(TokenType::Identifier);
    
    std::vector<std::unique_ptr<Expression>> arguments;
    
    if (Match(TokenType::LeftParen)) {
        if (!Check(TokenType::RightParen)) {
            arguments.push_back(ParseExpression());
            
            while (Match(TokenType::Comma)) {
                arguments.push_back(ParseExpression());
            }
        }
        Consume(TokenType::RightParen);
    } else if (Check(TokenType::String)) {
        std::string value = current_token_.GetValue();
        Advance();
        arguments.push_back(std::make_unique<StringLiteral>(value, GetCurrentPosition()));
    } else if (Check(TokenType::LeftBrace)) {
        arguments.push_back(ParseTableConstructor());
    }
    
    return std::make_unique<MethodCallExpression>(std::move(object), method, std::move(arguments), start_pos);
}

std::unique_ptr<TableConstructor> Parser::ParseTableConstructor() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::LeftBrace);
    
    std::vector<std::unique_ptr<TableField>> fields;
    
    if (!Check(TokenType::RightBrace)) {
        fields.push_back(ParseTableField());
        
        while (Match(TokenType::Comma) || Match(TokenType::Semicolon)) {
            if (Check(TokenType::RightBrace)) {
                break; // 允许末尾的逗号/分号
            }
            fields.push_back(ParseTableField());
        }
    }
    
    Consume(TokenType::RightBrace);
    
    return std::make_unique<TableConstructor>(std::move(fields), start_pos);
}

std::unique_ptr<TableField> Parser::ParseTableField() {
    SourcePosition start_pos = GetCurrentPosition();
    
    if (Check(TokenType::LeftBracket)) {
        // [key] = value
        Advance(); // consume '['
        auto key = ParseExpression();
        Consume(TokenType::RightBracket);
        Consume(TokenType::Assign);
        auto value = ParseExpression();
        return std::make_unique<TableField>(std::move(key), std::move(value), start_pos);
    } else if (Check(TokenType::Identifier) && PeekToken().GetType() == TokenType::Assign) {
        // name = value
        std::string name = current_token_.GetValue();
        Advance();
        Consume(TokenType::Assign);
        auto value = ParseExpression();
        auto key = std::make_unique<StringLiteral>(name, start_pos);
        return std::make_unique<TableField>(std::move(key), std::move(value), start_pos);
    } else {
        // array-style field: just value
        auto value = ParseExpression();
        return std::make_unique<TableField>(std::move(value), start_pos);
    }
}

std::unique_ptr<FunctionExpression> Parser::ParseFunctionExpression() {
    SourcePosition start_pos = GetCurrentPosition();
    
    Consume(TokenType::Function);
    
    bool is_vararg = false;
    auto parameters = ParseParameterList(is_vararg);
    
    auto body = ParseBlock();
    Consume(TokenType::End);
    
    return std::make_unique<FunctionExpression>(std::move(parameters), std::move(body), is_vararg, start_pos);
}

/* ========================================================================== */
/* 辅助解析方法 */
/* ========================================================================== */

std::unique_ptr<BlockNode> Parser::ParseBlock() {
    SourcePosition start_pos = GetCurrentPosition();
    
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!IsAtEnd() && !CheckAny({TokenType::End, TokenType::Else, TokenType::Elseif, 
                                    TokenType::Until, TokenType::EndOfSource})) {
        auto statement = ParseStatement();
        if (statement) {
            statements.push_back(std::move(statement));
        }
    }
    
    return std::make_unique<BlockNode>(std::move(statements), start_pos);
}

std::vector<std::string> Parser::ParseParameterList(bool& is_vararg) {
    std::vector<std::string> parameters;
    is_vararg = false;
    
    Consume(TokenType::LeftParen);
    
    if (!Check(TokenType::RightParen)) {
        if (Check(TokenType::Vararg)) {
            is_vararg = true;
            Advance();
        } else {
            parameters.push_back(current_token_.GetValue());
            Consume(TokenType::Identifier);
            
            while (Match(TokenType::Comma)) {
                if (Check(TokenType::Vararg)) {
                    is_vararg = true;
                    Advance();
                    break;
                } else {
                    parameters.push_back(current_token_.GetValue());
                    Consume(TokenType::Identifier);
                }
            }
        }
    }
    
    Consume(TokenType::RightParen);
    
    return parameters;
}

/* ========================================================================== */
/* 初始化和配置方法 */
/* ========================================================================== */

void Parser::InitializeOperatorTables() {
    // 已在其他方法中实现，这里保留接口
}

void Parser::InitializeState() {
    state_ = ParserState::Ready;
    error_count_ = 0;
    recursion_depth_ = 0;
    expression_depth_ = 0;
    recovery_strategy_ = RecoveryStrategy::Synchronize;
}

/* ========================================================================== */
/* 便利函数实现 */
/* ========================================================================== */

std::unique_ptr<Program> ParseLuaSource(const std::string& source,
                                       const std::string& filename,
                                       const ParserConfig& config) {
    auto stream = std::make_unique<StringInputStream>(source);
    auto lexer = std::make_unique<Lexer>(std::move(stream));
    auto parser = std::make_unique<Parser>(std::move(lexer), config);
    return parser->ParseProgram();
}

std::unique_ptr<Program> ParseLuaFile(const std::string& filename,
                                     const ParserConfig& config) {
    auto stream = std::make_unique<FileInputStream>(filename);
    auto lexer = std::make_unique<Lexer>(std::move(stream));
    auto parser = std::make_unique<Parser>(std::move(lexer), config);
    return parser->ParseProgram();
}

std::unique_ptr<Expression> ParseLuaExpression(const std::string& expression,
                                              const std::string& filename,
                                              const ParserConfig& config) {
    auto stream = std::make_unique<StringInputStream>(expression);
    auto lexer = std::make_unique<Lexer>(std::move(stream));
    auto parser = std::make_unique<Parser>(std::move(lexer), config);
    return parser->ParseExpression();
}

/* ========================================================================== */
/* 错误处理和恢复机制 */
/* ========================================================================== */

void Parser::ReportError(const std::string& message) {
    if (config_.use_enhanced_error_recovery && error_collector_) {
        // 使用增强错误恢复
        EnhancedSyntaxError enhanced_error(
            message,
            ErrorSeverity::Error,
            GetCurrentPosition(),
            ErrorCategory::Syntax
        );
        
        // 生成错误建议
        if (suggestion_generator_) {
            auto suggestions = suggestion_generator_->GenerateSuggestions(
                enhanced_error, current_token_, lexer_.get()
            );
            enhanced_error.SetSuggestions(suggestions);
        }
        
        error_collector_->AddError(enhanced_error);
        error_count_++;
        
        if (config_.recover_from_errors) {
            // 输出Lua 5.1.5兼容的错误格式
            if (error_formatter_) {
                std::cerr << error_formatter_->Format(enhanced_error) << std::endl;
            }
        } else {
            throw SyntaxError(message, GetCurrentPosition());
        }
    } else {
        // 传统错误处理
        error_count_++;
        
        std::stringstream ss;
        ss << "Parse error at line " << current_token_.GetLine() 
           << ", column " << current_token_.GetColumn() << ": " << message;
        
        if (config_.recover_from_errors) {
            // 记录错误但继续解析
            std::cerr << ss.str() << std::endl;
        } else {
            throw SyntaxError(message, GetCurrentPosition());
        }
    }
}

void Parser::ReportEnhancedError(const EnhancedSyntaxError& error) {
    if (error_collector_) {
        error_collector_->AddError(error);
    }
    error_count_++;
    
    if (config_.recover_from_errors) {
        if (error_formatter_) {
            std::cerr << error_formatter_->Format(error) << std::endl;
        }
    } else {
        throw SyntaxError(error.GetMessage(), error.GetPosition());
    }
}

std::vector<EnhancedSyntaxError> Parser::GetAllErrors() const {
    if (error_collector_) {
        return error_collector_->GetErrors();
    }
    return {};
}

ErrorContext Parser::CreateErrorContext() const {
    ErrorContext context;
    context.current_token = current_token_;
    context.position = GetCurrentPosition();
    context.recursion_depth = recursion_depth_;
    context.expression_depth = expression_depth_;
    context.parsing_state = state_;
    return context;
}

void Parser::SynchronizeTo(const std::vector<TokenType>& sync_tokens) {
    while (!IsAtEnd()) {
        if (std::find(sync_tokens.begin(), sync_tokens.end(), current_token_.GetType()) != sync_tokens.end()) {
            return;
        }
        Advance();
    }
}

void Parser::SynchronizeToNextStatement() {
    SynchronizeTo({TokenType::Local, TokenType::If, TokenType::While, 
                  TokenType::For, TokenType::Function, TokenType::Do,
                  TokenType::Break, TokenType::Return, TokenType::Semicolon,
                  TokenType::End, TokenType::Else, TokenType::Elseif,
                  TokenType::Until, TokenType::EndOfSource});
}

bool Parser::TryRecover() {
    if (config_.use_enhanced_error_recovery && recovery_engine_) {
        return TryEnhancedRecover(CreateErrorContext());
    }
    
    // 传统错误恢复
    switch (recovery_strategy_) {
        case RecoveryStrategy::None:
            return false;
            
        case RecoveryStrategy::SkipToNext:
            // 跳过到下一个语句开始
            SynchronizeTo({TokenType::Local, TokenType::If, TokenType::While, 
                          TokenType::For, TokenType::Function, TokenType::Do,
                          TokenType::Break, TokenType::Return, TokenType::Semicolon});
            return true;
            
        case RecoveryStrategy::InsertMissing:
            // TODO: 实现缺失token插入逻辑
            return false;
            
        case RecoveryStrategy::Synchronize:
            // 同步到块结束
            SynchronizeTo({TokenType::End, TokenType::Else, TokenType::Elseif,
                          TokenType::Until, TokenType::EndOfSource});
            return true;
            
        default:
            return false;
    }
}

bool Parser::TryEnhancedRecover(ErrorContext context) {
    if (!recovery_engine_) {
        return false;
    }
    
    // 检查错误数量限制
    if (error_count_ >= config_.max_errors) {
        return false;
    }
    
    auto recovery_actions = recovery_engine_->AnalyzeAndRecover(context);
    
    for (const auto& action : recovery_actions) {
        switch (action.type) {
            case RecoveryActionType::SkipToken:
                Advance();
                return true;
                
            case RecoveryActionType::InsertToken:
                // TODO: 实现token插入（可能需要修改Lexer接口）
                break;
                
            case RecoveryActionType::SynchronizeToKeyword:
                SynchronizeTo(action.sync_tokens);
                return true;
                
            case RecoveryActionType::RestartStatement:
                SynchronizeToNextStatement();
                return true;
                
            case RecoveryActionType::BacktrackAndRetry:
                // TODO: 实现回溯（需要token流状态保存）
                break;
        }
    }
    
    // 如果没有有效的恢复动作，使用传统恢复
    return false;
}

SourcePosition Parser::CreateErrorPosition() const {
    return GetCurrentPosition();
}

bool Parser::IsStatementStart(TokenType type) const {
    switch (type) {
        case TokenType::Local:
        case TokenType::If:
        case TokenType::While:
        case TokenType::Repeat:
        case TokenType::For:
        case TokenType::Function:
        case TokenType::Do:
        case TokenType::Break:
        case TokenType::Return:
        case TokenType::Identifier:
        case TokenType::LeftParen:
            return true;
        default:
            return false;
    }
}

bool Parser::IsExpressionStart(TokenType type) const {
    switch (type) {
        case TokenType::Nil:
        case TokenType::True:
        case TokenType::False:
        case TokenType::Number:
        case TokenType::String:
        case TokenType::Vararg:
        case TokenType::Identifier:
        case TokenType::LeftParen:
        case TokenType::LeftBrace:
        case TokenType::Function:
        case TokenType::Minus:
        case TokenType::Not:
        case TokenType::Length:
            return true;
        default:
            return false;
    }
}

void Parser::SkipOptionalSemicolon() {
    while (Match(TokenType::Semicolon)) {
        // 跳过所有连续的分号
    }
}

/* ========================================================================== */
/* 改进的错误恢复解析方法 */
/* ========================================================================== */

std::unique_ptr<Statement> Parser::ParseStatementWithRecovery() {
    try {
        return ParseStatement();
    } catch (const SyntaxError& e) {
        ReportError(e.GetMessage());
        
        if (TryRecover()) {
            // 尝试继续解析
            return nullptr;
        } else {
            throw;
        }
    }
}

std::unique_ptr<Expression> Parser::ParseExpressionWithRecovery() {
    try {
        return ParseExpression();
    } catch (const SyntaxError& e) {
        ReportError(e.GetMessage());
        
        if (TryRecover()) {
            // 返回错误占位符表达式
            return std::make_unique<NilLiteral>(GetCurrentPosition());
        } else {
            throw;
        }
    }
}

} // namespace lua_cpp