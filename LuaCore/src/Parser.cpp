#include "Parser.h"
#include "String.h"
#include <stdexcept>
#include <iostream>

namespace LuaCore {

Parser::Parser(Lexer& lexer) : m_lexer(lexer) {
    // 初始化当前标记
    advance();
}

void Parser::advance() {
    m_current = m_lexer.nextToken();
    
    // 如果遇到错误标记，立即抛出异常
    if (m_current.type == TokenType::Error) {
        throw error(m_current.lexeme);
    }
}

bool Parser::check(TokenType type) {
    return m_current.type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

void Parser::consume(TokenType type, const Str& message) {
    if (check(type)) {
        advance();
        return;
    }
    
    throw error(message);
}

ParseError Parser::error(const Str& message) {
    m_panicMode = true;
    return ParseError(message, m_current.line, m_current.column);
}

TokenType Parser::peekNext() {
    return m_lexer.peek().type;
}

TokenType Parser::peekNextNext() {
    Token first = m_lexer.peek();
    m_lexer.saveLexerState();
    m_lexer.nextToken(); // 消耗一个标记
    Token second = m_lexer.peek();
    m_lexer.restoreLexerState();
    return second.type;
}

void Parser::synchronize() {
    m_panicMode = false;
    
    while (m_current.type != TokenType::EndOfFile) {
        // 语句边界标记
        if (m_current.type == TokenType::Semicolon) {
            advance();
            return;
        }
        
        // 语句开始关键字
        switch (m_current.type) {
            case TokenType::Function:
            case TokenType::Local:
            case TokenType::If:
            case TokenType::For:
            case TokenType::While:
            case TokenType::Repeat:
            case TokenType::Return:
                return;
            default:
                break;
        }
        
        advance();
    }
}

std::shared_ptr<Block> Parser::parse() {
    try {
        return block();
    } catch (const ParseError& e) {
        // 在这里可以添加错误处理逻辑
        // 例如打印错误信息、收集错误等
        
        // 重新抛出异常或返回部分结果
        throw;
    }
}

std::shared_ptr<Block> Parser::block() {
    auto blockNode = std::make_shared<Block>();
    
    while (!check(TokenType::EndOfFile) && 
           !check(TokenType::End) && 
           !check(TokenType::Else) && 
           !check(TokenType::Elseif) && 
           !check(TokenType::Until)) {
        try {
            blockNode->addStatement(statement());
        } catch (const ParseError& e) {
            if (m_panicMode) {
                synchronize();
            } else {
                throw;
            }
        }
    }
    
    return blockNode;
}

std::shared_ptr<Statement> Parser::statement() {
    // 处理各种语句类型
    if (match(TokenType::If)) {
        return ifStatement();
    }
    if (match(TokenType::While)) {
        return whileStatement();
    }
    if (match(TokenType::Do)) {
        return doStatement();
    }
    if (match(TokenType::For)) {
        return forStatement();
    }
    if (match(TokenType::Repeat)) {
        return repeatStatement();
    }
    if (match(TokenType::Function)) {
        return functionStatement();
    }
    if (match(TokenType::Local)) {
        return localStatement();
    }
    if (match(TokenType::Return)) {
        return returnStatement();
    }
    if (match(TokenType::Break)) {
        return breakStatement();
    }
    
    // 普通语句（赋值或函数调用）
    return assignOrCallStatement();
}

std::shared_ptr<Statement> Parser::assignOrCallStatement() {
    // 这里需要先解析表达式，然后判断是赋值还是函数调用
    
    // 解析前缀表达式
    auto expr = suffixedExpr();
    
    // 如果是函数调用，直接返回函数调用语句
    if (auto call = std::dynamic_pointer_cast<FunctionCallExpr>(expr)) {
        return std::make_shared<FunctionCallStmt>(call);
    }
    
    // 如果是赋值，需要处理左值列表和右值列表
    if (match(TokenType::Assign)) {
        std::vector<std::shared_ptr<Expression>> vars;
        vars.push_back(expr);
        
        // 解析多重赋值的左值列表
        while (match(TokenType::Comma)) {
            vars.push_back(suffixedExpr());
        }
        
        // 解析右值列表
        consume(TokenType::Assign, Str("Expected '=' in assignment"));
        auto values = exprList();
        
        return std::make_shared<AssignmentStmt>(vars, values);
    }
    
    // 如果不是函数调用也不是赋值，报错
    throw error(Str("Expected assignment or function call"));
}

std::shared_ptr<Statement> Parser::localStatement() {
    if (match(TokenType::Function)) {
        // 局部函数声明
        return functionStatement();
    } else {
        // 局部变量声明
        auto names = nameList();
        std::vector<std::shared_ptr<Expression>> initializers;
        
        // 检查是否有初始化表达式
        if (match(TokenType::Assign)) {
            initializers = exprList();
        }
        
        return std::make_shared<LocalVarDeclStmt>(names, initializers);
    }
}

std::vector<Str> Parser::nameList() {
    std::vector<Str> names;
    
    // 第一个名称
    if (check(TokenType::Identifier)) {
        names.push_back(m_current.lexeme);
        advance();
    } else {
        throw error(Str("Expected identifier"));
    }
    
    // 后续名称
    while (match(TokenType::Comma)) {
        if (check(TokenType::Identifier)) {
            names.push_back(m_current.lexeme);
            advance();
        } else {
            throw error(Str("Expected identifier after ','"));
        }
    }
    
    return names;
}

std::vector<std::shared_ptr<Expression>> Parser::exprList() {
    std::vector<std::shared_ptr<Expression>> exprs;
    
    // 至少一个表达式
    exprs.push_back(expression());
    
    // 后续表达式
    while (match(TokenType::Comma)) {
        exprs.push_back(expression());
    }
    
    return exprs;
}

std::shared_ptr<Statement> Parser::ifStatement() {
    // 主分支条件
    auto condition = expression();
    consume(TokenType::Then, Str("Expected 'then' after if condition"));
    
    // 主分支代码块
    auto thenBranch = block();
    
    // 收集elseif分支
    std::vector<IfStmt::Branch> elseIfBranches;
    while (match(TokenType::Elseif)) {
        auto elseIfCondition = expression();
        consume(TokenType::Then, Str("Expected 'then' after elseif condition"));
        auto elseIfBlock = block();
        elseIfBranches.push_back({elseIfCondition, elseIfBlock});
    }
    
    // else分支（可选）
    std::shared_ptr<Block> elseBranch;
    if (match(TokenType::Else)) {
        elseBranch = block();
    }
    
    // 结束if语句
    consume(TokenType::End, Str("Expected 'end' to close if statement"));
    
    return std::make_shared<IfStmt>(condition, thenBranch, elseIfBranches, elseBranch);
}

std::shared_ptr<Statement> Parser::whileStatement() {
    // 条件
    auto condition = expression();
    consume(TokenType::Do, Str("Expected 'do' after while condition"));
    
    // 循环体
    auto body = block();
    
    // 结束while语句
    consume(TokenType::End, Str("Expected 'end' to close while statement"));
    
    return std::make_shared<WhileStmt>(condition, body);
}

std::shared_ptr<Statement> Parser::doStatement() {
    // do语句只是一个普通代码块
    auto body = block();
    
    // 结束do语句
    consume(TokenType::End, Str("Expected 'end' to close do statement"));
    
    return std::make_shared<DoStmt>(body);
}

std::shared_ptr<Statement> Parser::forStatement() {
    // 循环变量名
    Str name;
    if (check(TokenType::Identifier)) {
        name = m_current.lexeme;
        advance();
    } else {
        throw error(Str("Expected identifier in for statement"));
    }
    
    // 数值for循环
    if (match(TokenType::Assign)) {
        auto start = expression();
        consume(TokenType::Comma, Str("Expected ',' after initial value in for loop"));
        auto end = expression();
        
        // 步长（可选）
        std::shared_ptr<Expression> step;
        if (match(TokenType::Comma)) {
            step = expression();
        } else {
            // 默认步长为1
            step = std::make_shared<LiteralExpr>(Value(1.0));
        }
        
        consume(TokenType::Do, Str("Expected 'do' after for loop conditions"));
        auto body = block();
        consume(TokenType::End, Str("Expected 'end' to close for loop"));
        
        return std::make_shared<NumericForStmt>(name, start, end, step, body);
    }
    // 泛型for循环
    else if (match(TokenType::Comma) || match(TokenType::In)) {
        std::vector<Str> vars;
        vars.push_back(name);
        
        // 如果是逗号，继续收集变量名
        if (m_current.type == TokenType::Comma) {
            while (match(TokenType::Comma)) {
                if (check(TokenType::Identifier)) {
                    vars.push_back(m_current.lexeme);
                    advance();
                } else {
                    throw error(Str("Expected identifier after ',' in for loop"));
                }
            }
        }
        
        // in后面跟迭代器表达式
        consume(TokenType::In, Str("Expected 'in' in generic for loop"));
        auto iterators = exprList();
        
        consume(TokenType::Do, Str("Expected 'do' after for loop conditions"));
        auto body = block();
        consume(TokenType::End, Str("Expected 'end' to close for loop"));
        
        return std::make_shared<GenericForStmt>(vars, iterators, body);
    } else {
        throw error(Str("Expected '=' or 'in' after variable name in for loop"));
    }
}

std::shared_ptr<Statement> Parser::repeatStatement() {
    // 循环体
    auto body = block();
    
    // until条件
    consume(TokenType::Until, Str("Expected 'until' after repeat block"));
    auto condition = expression();
    
    return std::make_shared<RepeatStmt>(body, condition);
}

std::shared_ptr<Statement> Parser::functionStatement() {
    bool isLocal = m_current.type == TokenType::Local;
    
    // 如果当前标记是local，需要前进到function
    if (isLocal) {
        advance();
        consume(TokenType::Function, Str("Expected 'function' after 'local'"));
    }
    
    // 解析函数名
    std::vector<Str> nameComponents;
    
    // 第一个名称部分
    if (check(TokenType::Identifier)) {
        nameComponents.push_back(m_current.lexeme);
        advance();
    } else {
        throw error(Str("Expected function name"));
    }
    
    // 后续名称部分 (a.b.c)
    bool isMethod = false;
    while (match(TokenType::Dot)) {
        if (check(TokenType::Identifier)) {
            nameComponents.push_back(m_current.lexeme);
            advance();
        } else {
            throw error(Str("Expected identifier after '.' in function name"));
        }
    }
    
    // 检查是否是方法定义 (a:b)
    if (match(TokenType::Colon)) {
        isMethod = true;
        if (check(TokenType::Identifier)) {
            nameComponents.push_back(m_current.lexeme);
            advance();
        } else {
            throw error(Str("Expected method name after ':'"));
        }
    }
    
    // 解析参数列表
    consume(TokenType::LeftParen, Str("Expected '(' after function name"));
    bool isVararg = false;
    auto params = paramList(isVararg);
    consume(TokenType::RightParen, Str("Expected ')' after function parameters"));
    
    // 解析函数体
    auto body = block();
    consume(TokenType::End, Str("Expected 'end' to close function definition"));
    
    return std::make_shared<FunctionDeclStmt>(
        nameComponents, isLocal, isMethod, params, isVararg, body);
}

std::vector<Str> Parser::paramList(bool& isVararg) {
    std::vector<Str> params;
    isVararg = false;
    
    // 空参数列表
    if (check(TokenType::RightParen)) {
        return params;
    }
    
    // 变长参数
    if (check(TokenType::Dot) && peekNext() == TokenType::Dot && peekNextNext() == TokenType::Dot) {
        advance(); // 消耗第一个'.'
        advance(); // 消耗第二个'.'
        advance(); // 消耗第三个'.'
        isVararg = true;
        return params;
    }
    
    // 第一个参数
    if (check(TokenType::Identifier)) {
        params.push_back(m_current.lexeme);
        advance();
    } else {
        throw error(Str("Expected parameter name or '...'"));
    }
    
    // 后续参数
    while (match(TokenType::Comma)) {
        // 检查是否是变长参数
        if (check(TokenType::Dot) && peekNext() == TokenType::Dot && peekNextNext() == TokenType::Dot) {
            advance(); // 消耗第一个'.'
            advance(); // 消耗第二个'.'
            advance(); // 消耗第三个'.'
            isVararg = true;
            break;
        }
        
        if (check(TokenType::Identifier)) {
            params.push_back(m_current.lexeme);
            advance();
        } else {
            throw error(Str("Expected parameter name or '...' after ','"));
        }
    }
    
    return params;
}

std::shared_ptr<Statement> Parser::returnStatement() {
    std::vector<std::shared_ptr<Expression>> values;
    
    // 如果不是语句结束标记，则解析返回值列表
    if (!check(TokenType::End) && 
        !check(TokenType::Else) && 
        !check(TokenType::Elseif) && 
        !check(TokenType::Until) && 
        !check(TokenType::EndOfFile)) {
        values = exprList();
    }
    
    // 返回语句后面可能有分号
    match(TokenType::Semicolon);
    
    return std::make_shared<ReturnStmt>(values);
}

std::shared_ptr<Statement> Parser::breakStatement() {
    // break语句后面可能有分号
    match(TokenType::Semicolon);
    
    return std::make_shared<BreakStmt>();
}

std::shared_ptr<Expression> Parser::expression() {
    return orExpr();
}

std::shared_ptr<Expression> Parser::orExpr() {
    auto expr = andExpr();
    
    while (match(TokenType::Or)) {
        auto right = andExpr();
        expr = std::make_shared<BinaryExpr>(BinaryExpr::Op::Or, expr, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::andExpr() {
    auto expr = comparisonExpr();
    
    while (match(TokenType::And)) {
        auto right = comparisonExpr();
        expr = std::make_shared<BinaryExpr>(BinaryExpr::Op::And, expr, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::comparisonExpr() {
    auto expr = concatExpr();
    
    while (true) {
        BinaryExpr::Op op;
        
        if (match(TokenType::Equal)) op = BinaryExpr::Op::Equal;
        else if (match(TokenType::NotEqual)) op = BinaryExpr::Op::NotEqual;
        else if (match(TokenType::LessThan)) op = BinaryExpr::Op::LessThan;
        else if (match(TokenType::LessEqual)) op = BinaryExpr::Op::LessEqual;
        else if (match(TokenType::GreaterThan)) op = BinaryExpr::Op::GreaterThan;
        else if (match(TokenType::GreaterEqual)) op = BinaryExpr::Op::GreaterEqual;
        else break;
        
        auto right = concatExpr();
        expr = std::make_shared<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::concatExpr() {
    auto expr = additiveExpr();
    
    if (match(TokenType::Concat)) {
        // 字符串连接操作符是右结合的
        auto right = concatExpr();
        expr = std::make_shared<BinaryExpr>(BinaryExpr::Op::Concat, expr, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::additiveExpr() {
    auto expr = multiplicativeExpr();
    
    while (true) {
        BinaryExpr::Op op;
        
        if (match(TokenType::Plus)) op = BinaryExpr::Op::Add;
        else if (match(TokenType::Minus)) op = BinaryExpr::Op::Subtract;
        else break;
        
        auto right = multiplicativeExpr();
        expr = std::make_shared<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::multiplicativeExpr() {
    auto expr = unaryExpr();
    
    while (true) {
        BinaryExpr::Op op;
        
        if (match(TokenType::Star)) op = BinaryExpr::Op::Multiply;
        else if (match(TokenType::Slash)) op = BinaryExpr::Op::Divide;
        else if (match(TokenType::Percent)) op = BinaryExpr::Op::Modulo;
        else break;
        
        auto right = unaryExpr();
        expr = std::make_shared<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::unaryExpr() {
    if (match(TokenType::Minus)) {
        auto expr = unaryExpr();
        return std::make_shared<UnaryExpr>(UnaryExpr::Op::Negate, expr);
    } else if (match(TokenType::Not)) {
        auto expr = unaryExpr();
        return std::make_shared<UnaryExpr>(UnaryExpr::Op::Not, expr);
    } else if (match(TokenType::Hash)) {
        auto expr = unaryExpr();
        return std::make_shared<UnaryExpr>(UnaryExpr::Op::Length, expr);
    }
    
    return powerExpr();
}

std::shared_ptr<Expression> Parser::powerExpr() {
    auto expr = simpleExpr();
    
    if (match(TokenType::Caret)) {
        // 幂运算符是右结合的
        auto right = powerExpr();
        expr = std::make_shared<BinaryExpr>(BinaryExpr::Op::Power, expr, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::simpleExpr() {
    if (check(TokenType::Nil)) {
        advance();
        return std::make_shared<LiteralExpr>(Value());
    } else if (check(TokenType::True)) {
        advance();
        return std::make_shared<LiteralExpr>(Value(true));
    } else if (check(TokenType::False)) {
        advance();
        return std::make_shared<LiteralExpr>(Value(false));
    } else if (check(TokenType::Number)) {
        double value = m_current.numberValue;
        advance();
        return std::make_shared<LiteralExpr>(Value(value));
    } else if (check(TokenType::String)) {
        Str value = m_current.stringValue;
        advance();
        return std::make_shared<LiteralExpr>(Value(value));
    } else if (match(TokenType::Function)) {
        return functionExpr();
    } else if (match(TokenType::LeftBrace)) {
        return tableConstructorExpr();
    } else if (match(TokenType::LeftParen)) {
        auto expr = expression();
        consume(TokenType::RightParen, Str("Expected ')' after expression"));
        return expr;
    } else if (check(TokenType::Identifier)) {
        Str name = m_current.lexeme;
        advance();
        return std::make_shared<VariableExpr>(name);
    }
    
    throw error(Str("Expected expression"));
}

std::shared_ptr<Expression> Parser::suffixedExpr() {
    auto expr = primaryExpr();
    
    while (true) {
        if (match(TokenType::Dot)) {
            // 字段访问 (t.k)
            if (check(TokenType::Identifier)) {
                Str field = m_current.lexeme;
                advance();
                expr = std::make_shared<FieldAccessExpr>(expr, field);
            } else {
                throw error(Str("Expected identifier after '.'"));
            }
        } else if (match(TokenType::LeftBracket)) {
            // 表访问 (t[k])
            auto key = expression();
            consume(TokenType::RightBracket, Str("Expected ']' after table key"));
            expr = std::make_shared<TableAccessExpr>(expr, key);
        } else if (match(TokenType::Colon)) {
            // 方法调用 (t:m())
            if (check(TokenType::Identifier)) {
                Str method = m_current.lexeme;
                advance();
                expr = std::make_shared<FieldAccessExpr>(expr, method);
                expr = functionCall(expr);
            } else {
                throw error(Str("Expected method name after ':'"));
            }
        } else if (check(TokenType::LeftParen) || check(TokenType::String) || check(TokenType::LeftBrace)) {
            // 函数调用 (f())
            expr = functionCall(expr);
        } else {
            break;
        }
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::primaryExpr() {
    if (check(TokenType::Identifier)) {
        Str name = m_current.lexeme;
        advance();
        return std::make_shared<VariableExpr>(name);
    } else {
        return simpleExpr();
    }
}

std::shared_ptr<FunctionCallExpr> Parser::functionCall(std::shared_ptr<Expression> prefix) {
    std::shared_ptr<ExpressionList> args = std::make_shared<ExpressionList>();
    
    if (match(TokenType::LeftParen)) {
        // 普通函数调用 f(args)
        if (!check(TokenType::RightParen)) {
            // 有参数
            auto exprList = this->exprList();
            for (const auto& expr : exprList) {
                args->addExpression(expr);
            }
        }
        consume(TokenType::RightParen, Str("Expected ')' after function arguments"));
    } else if (check(TokenType::String)) {
        // 字符串参数调用 f "str"
        args->addExpression(std::make_shared<LiteralExpr>(Value(m_current.stringValue)));
        advance();
    } else if (check(TokenType::LeftBrace)) {
        // 表构造器参数调用 f {fields}
        args->addExpression(tableConstructorExpr());
    } else {
        throw error(Str("Expected '(', string, or table after function"));
    }
    
    return std::make_shared<FunctionCallExpr>(prefix, args);
}

std::shared_ptr<Expression> Parser::functionExpr() {
    consume(TokenType::LeftParen, Str("Expected '(' after 'function'"));
    
    // 解析参数列表
    bool isVararg = false;
    auto params = paramList(isVararg);
    
    consume(TokenType::RightParen, Str("Expected ')' after function parameters"));
    
    // 解析函数体
    auto body = block();
    
    consume(TokenType::End, Str("Expected 'end' to close function definition"));
    
    return std::make_shared<FunctionDefExpr>(params, isVararg, body);
}

std::shared_ptr<Expression> Parser::tableConstructorExpr() {
    auto table = std::make_shared<TableConstructorExpr>();
    
    // 空表
    if (match(TokenType::RightBrace)) {
        return table;
    }
    
    // 解析第一个字段
    TableConstructorExpr::Field field = parseTableField();
    table->addField(field);
    
    // 解析后续字段
    while (match(TokenType::Comma) || match(TokenType::Semicolon)) {
        if (check(TokenType::RightBrace)) {
            break; // 允许结尾的逗号或分号
        }
        
        field = parseTableField();
        table->addField(field);
    }
    
    consume(TokenType::RightBrace, Str("Expected '}' to close table constructor"));
    
    return table;
}

TableConstructorExpr::Field Parser::parseTableField() {
    TableConstructorExpr::Field field;
    
    if (match(TokenType::LeftBracket)) {
        // [expr] = expr
        field.key = expression();
        consume(TokenType::RightBracket, Str("Expected ']' after table key"));
        consume(TokenType::Assign, Str("Expected '=' after table key"));
        field.value = expression();
    } else if (check(TokenType::Identifier) && peekNext() == TokenType::Assign) {
        // name = expr
        Str name = m_current.lexeme;
        advance();
        consume(TokenType::Assign, Str("Expected '=' after field name"));
        
        field.key = std::make_shared<LiteralExpr>(Value(name));
        field.value = expression();
    } else {
        // expr (数组项)
        field.key = nullptr; // 数组项没有显式的键
        field.value = expression();
    }
    
    return field;
}

std::shared_ptr<ExpressionList> Parser::expressionList() {
    auto list = std::make_shared<ExpressionList>();
    
    // 至少一个表达式
    list->addExpression(expression());
    
    // 后续表达式
    while (match(TokenType::Comma)) {
        list->addExpression(expression());
    }
    
    return list;
}

} // namespace LuaCore
