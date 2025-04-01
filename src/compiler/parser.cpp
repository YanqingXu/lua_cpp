#include "parser.hpp"
#include "compiler/ast.hpp"

namespace Lua {

Parser::Parser(Lexer& lexer) : m_lexer(lexer) {
    // 初始化解析器状态
    advance(); // 读取第一个token
}

Ptr<Block> Parser::parse() {
    try {
        // 创建顶层代码块
        auto mainBlock = make_ptr<Block>();
        
        // 解析语句序列直到文件结束
        while (!match(TokenType::Eof)) {
            if (auto stmt = parseStatement()) {
                mainBlock->addStatement(stmt);
            }
            
            // 允许在语句之间有可选的分号
            while (match(TokenType::Semicolon)) {
                // 跳过所有连续的分号
            }
        }
        
        return mainBlock;
    }
    catch (const ParseError& error) {
        // 解析错误处理
        // 记录错误并尝试同步
        synchronize();
        return nullptr;
    }
}

void Parser::advance() {
    m_previous = m_current;
    m_current = m_lexer.nextToken();
    
    // 如果遇到错误token，抛出异常
    if (m_current.type == TokenType::Error) {
        error(m_current.lexeme);
    }
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    return m_current.type == type;
}

Token Parser::consume(TokenType type, const Str& message) {
    if (check(type)) {
        Token token = m_current;
        advance();
        return token;
    }
    
    error(message);
    // 这里不会执行到，因为error会抛出异常
    return m_current;
}

void Parser::error(const Str& message) {
    // 抛出解析错误，包含当前位置信息
    throw ParseError(message, m_current.line, m_current.column);
}

void Parser::synchronize() {
    // 错误恢复：跳过tokens直到遇到可能是语句开始的token
    advance();
    
    while (!check(TokenType::Eof)) {
        // 如果上一个token是分号，可能表示语句结束
        if (m_previous.type == TokenType::Semicolon) return;
        
        // 根据当前token类型检查是否可能是新语句的开始
        switch (m_current.type) {
            case TokenType::Function:
            case TokenType::Local:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Do:
            case TokenType::For:
            case TokenType::Repeat:
            case TokenType::Return:
                return;
            default:
                // 继续跳过
                break;
        }
        
        advance();
    }
}

Ptr<Statement> Parser::parseStatement() {
    // 根据当前token判断语句类型
    if (match(TokenType::If)) {
        return parseIfStatement();
    }
    if (match(TokenType::While)) {
        return parseWhileStatement();
    }
    if (match(TokenType::Do)) {
        return parseDoStatement();
    }
    if (match(TokenType::For)) {
        // 区分数值for和通用for
        if (match(TokenType::Identifier)) {
            // 保存当前状态
            Token name = m_previous;
            if (match(TokenType::Equal)) {
                // 数值for
                advance(); // 恢复状态
                return parseNumericForStatement(name.lexeme);
            } else {
                // 通用for
                Vec<Str> names;
                names.push_back(name.lexeme);
                
                // 解析更多变量名
                while (match(TokenType::Comma)) {
                    if (match(TokenType::Identifier)) {
                        names.push_back(m_previous.lexeme);
                    } else {
                        error("for循环变量名必须是标识符");
                    }
                }
                
                consume(TokenType::In, "for循环缺少'in'关键字");
                return parseGenericForStatement(names);
            }
        } else {
            error("for循环缺少变量名");
            return nullptr;
        }
    }
    if (match(TokenType::Repeat)) {
        return parseRepeatStatement();
    }
    if (match(TokenType::Function)) {
        return parseFunctionDeclaration(false);
    }
    if (match(TokenType::Local)) {
        if (check(TokenType::Function)) {
            advance(); // 消费function关键字
            return parseFunctionDeclaration(true);
        } else {
            return parseLocalVariableDeclaration();
        }
    }
    if (match(TokenType::Return)) {
        return parseReturnStatement();
    }
    if (match(TokenType::Break)) {
        return parseBreakStatement();
    }
    
    // 如果不是特定的语句关键字，则可能是赋值语句或函数调用
    return parseAssignmentOrCallStatement();
}

Ptr<Expression> Parser::parseExpression() {
    // 表达式解析从最低优先级的操作符开始
    return parseOr();
}

Ptr<Expression> Parser::parseOr() {
    // 解析左侧表达式
    auto expr = parseAnd();
    
    // 循环检查是否有OR运算符
    while (match(TokenType::Or)) {
        auto right = parseAnd();
        expr = make_ptr<BinaryExpr>(BinaryExpr::Op::Or, expr, right);
    }
    
    return expr;
}

Ptr<Expression> Parser::parseAnd() {
    // 解析左侧表达式
    auto expr = parseComparison();
    
    // 循环检查是否有AND运算符
    while (match(TokenType::And)) {
        auto right = parseComparison();
        expr = make_ptr<BinaryExpr>(BinaryExpr::Op::And, expr, right);
    }
    
    return expr;
}

Ptr<Expression> Parser::parseComparison() {
    // 解析左侧表达式
    auto expr = parseConcat();
    
    // 循环检查是否有比较运算符
    while (match(TokenType::Equal) || match(TokenType::NotEqual) ||
           match(TokenType::Less) || match(TokenType::LessEqual) ||
           match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
        BinaryExpr::Op op;
        
        switch (m_previous.type) {
            case TokenType::Equal:        op = BinaryExpr::Op::Equal; break;
            case TokenType::NotEqual:     op = BinaryExpr::Op::NotEqual; break;
            case TokenType::Less:         op = BinaryExpr::Op::LessThan; break;
            case TokenType::LessEqual:    op = BinaryExpr::Op::LessEqual; break;
            case TokenType::Greater:      op = BinaryExpr::Op::GreaterThan; break;
            case TokenType::GreaterEqual: op = BinaryExpr::Op::GreaterEqual; break;
            default: error("无效的比较运算符"); return expr;
        }
        
        auto right = parseConcat();
        expr = make_ptr<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

Ptr<Expression> Parser::parseConcat() {
    // 解析左侧表达式
    auto expr = parseAdditive();
    
    // 循环检查是否有连接运算符
    while (match(TokenType::Concat)) {
        auto right = parseAdditive();
        expr = make_ptr<BinaryExpr>(BinaryExpr::Op::Concat, expr, right);
    }
    
    return expr;
}

Ptr<Expression> Parser::parseAdditive() {
    // 解析左侧表达式
    auto expr = parseMultiplicative();
    
    // 循环检查是否有加减运算符
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        BinaryExpr::Op op = (m_previous.type == TokenType::Plus) 
                               ? BinaryExpr::Op::Add 
                               : BinaryExpr::Op::Subtract;
        auto right = parseMultiplicative();
        expr = make_ptr<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

Ptr<Expression> Parser::parseMultiplicative() {
    // 解析左侧表达式
    auto expr = parseUnary();
    
    // 循环检查是否有乘除模运算符
    while (match(TokenType::Star) || match(TokenType::Slash) || 
           match(TokenType::Percent) || match(TokenType::DoubleSlash)) {
        BinaryExpr::Op op;
        
        switch (m_previous.type) {
            case TokenType::Star:       op = BinaryExpr::Op::Multiply; break;
            case TokenType::Slash:      op = BinaryExpr::Op::Divide; break;
            case TokenType::Percent:    op = BinaryExpr::Op::Modulo; break;
            case TokenType::DoubleSlash: op = BinaryExpr::Op::FloorDivide; break;
            default: error("无效的乘法运算符"); return expr;
        }
        
        auto right = parseUnary();
        expr = make_ptr<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

Ptr<Expression> Parser::parseUnary() {
    // 检查是否有一元运算符
    if (match(TokenType::Not) || match(TokenType::Minus) || match(TokenType::Hash)) {
        UnaryExpr::Op op;
        
        switch (m_previous.type) {
            case TokenType::Not:      op = UnaryExpr::Op::Not; break;
            case TokenType::Minus:    op = UnaryExpr::Op::Negate; break;
            case TokenType::Hash:  op = UnaryExpr::Op::Length; break;
            default: error("无效的一元运算符"); return nullptr;
        }
        
        auto right = parseUnary();
        return make_ptr<UnaryExpr>(op, right);
    }
    
    // 如果没有一元运算符，解析幂运算
    return parsePower();
}

Ptr<Expression> Parser::parsePower() {
    // 解析左侧表达式
    auto expr = parsePrimary();
    
    // 检查是否有幂运算符
    if (match(TokenType::Power)) {
        auto right = parseUnary(); // 幂运算是右结合的
        expr = make_ptr<BinaryExpr>(BinaryExpr::Op::Power, expr, right);
    }
    
    return expr;
}

Ptr<Expression> Parser::parsePrimary() {
    // 解析基本表达式
    if (match(TokenType::Nil)) {
        return make_ptr<NilExpr>();
    }
    
    if (match(TokenType::True)) {
        return make_ptr<BoolExpr>(true);
    }
    
    if (match(TokenType::False)) {
        return make_ptr<BoolExpr>(false);
    }
    
    if (match(TokenType::Number)) {
        double value = std::stod(m_previous.lexeme);
        return make_ptr<NumberExpr>(value);
    }
    
    if (match(TokenType::String)) {
        return make_ptr<StringExpr>(m_previous.lexeme);
    }
    
    if (match(TokenType::LeftBrace)) {
        return parseTableConstructor();
    }
    
    if (match(TokenType::Function)) {
        return parseFunctionLiteral();
    }
    
    if (match(TokenType::LeftParen)) {
        auto expr = parseExpression();
        consume(TokenType::RightParen, "表达式后缺少右括号");
        return expr;
    }
    
    if (match(TokenType::Identifier)) {
        // 变量引用或函数调用
        auto var = make_ptr<VariableExpr>(m_previous.lexeme);
        
        // 检查是否是函数调用或表索引
        if (check(TokenType::LeftParen) || check(TokenType::LeftBracket) || 
            check(TokenType::Dot) || check(TokenType::Colon)) {
            return parseCall(var);
        }
        
        return var;
    }
    
    error("无效的表达式");
    return nullptr;
}

Ptr<IfStmt> Parser::parseIfStatement() {
    // 解析条件表达式
    auto condition = parseExpression();
    consume(TokenType::Then, "if条件后缺少'then'关键字");
    
    // 解析if块
    auto thenBlock = parseBlock();
    
    // 解析elseif和else块
    Vec<std::pair<Ptr<Expression>, Ptr<Block>>> elseifBlocks;
    Ptr<Block> elseBlock = nullptr;
    
    while (match(TokenType::Elseif)) {
        auto elseifCondition = parseExpression();
        consume(TokenType::Then, "elseif条件后缺少'then'关键字");
        auto elseifBody = parseBlock();
        elseifBlocks.push_back({elseifCondition, elseifBody});
    }
    
    if (match(TokenType::Else)) {
        elseBlock = parseBlock();
    }
    
    consume(TokenType::End, "if语句后缺少'end'关键字");
    
    return make_ptr<IfStmt>(condition, thenBlock, elseifBlocks, elseBlock);
}

Ptr<Block> Parser::parseBlock() {
    // 创建一个新的代码块
    auto block = make_ptr<Block>();
    
    // 解析语句序列直到块结束标记
    while (!check(TokenType::End) && !check(TokenType::Elseif) && 
           !check(TokenType::Else) && !check(TokenType::Until) && 
           !check(TokenType::Eof)) {
        if (auto stmt = parseStatement()) {
            block->addStatement(stmt);
        }
        
        // 允许在语句之间有可选的分号
        while (match(TokenType::Semicolon)) {
            // 跳过所有连续的分号
        }
    }
    
    return block;
}

Ptr<TableConstructorExpr> Parser::parseTableConstructor() {
    // 创建表构造器
    auto table = make_ptr<TableConstructorExpr>();
    
    // 解析表字段
    if (!check(TokenType::RightBrace)) {
        do {
            // 解析字段：[expr]=expr 或 name=expr 或 expr
            TableConstructorExpr::Field field;
            
            if (match(TokenType::LeftBracket)) {
                // [expr]=expr 形式
                field.key = parseExpression();
                consume(TokenType::RightBracket, "表键值后缺少右中括号");
                consume(TokenType::Equal, "表键值对缺少等号");
                field.value = parseExpression();
            } else if (match(TokenType::Identifier) && check(TokenType::Equal)) {
                // name=expr 形式
                Str name = m_previous.lexeme;
                advance(); // 消费等号
                field.key = make_ptr<LiteralExpr>(Object::Value::string(name));
                field.value = parseExpression();
            } else {
                // expr 形式（数组元素）
                field.value = parseExpression();
            }
            
            table->addField(field);
            
            // 字段之间可以有逗号或分号
        } while (match(TokenType::Comma) || match(TokenType::Semicolon));
    }
    
    consume(TokenType::RightBrace, "表构造器缺少右大括号");
    
    return table;
}

Ptr<FunctionDefExpr> Parser::parseFunctionLiteral() {
    // 解析函数参数列表
    consume(TokenType::LeftParen, "函数定义缺少左括号");
    
    // 解析参数名列表
    Vec<Str> params;
    bool isVararg = false;
    
    if (!check(TokenType::RightParen)) {
        if (match(TokenType::Dots)) {
            isVararg = true;
        } else if (match(TokenType::Identifier)) {
            params.push_back(m_previous.lexeme);
            
            while (match(TokenType::Comma)) {
                if (match(TokenType::Dots)) {
                    isVararg = true;
                    break;
                } else if (match(TokenType::Identifier)) {
                    params.push_back(m_previous.lexeme);
                } else {
                    error("函数参数必须是标识符");
                }
            }
        }
    }
    
    consume(TokenType::RightParen, "函数参数列表缺少右括号");
    
    // 解析函数体
    auto body = parseBlock();
    
    consume(TokenType::End, "函数定义缺少'end'关键字");
    
    return make_ptr<FunctionDefExpr>(params, isVararg, body);
}

Ptr<FunctionCallExpr> Parser::parseCall(Ptr<Expression> callee) {
    // 解析参数列表
    auto args = make_ptr<ExpressionList>();
    
    if (match(TokenType::LeftParen)) {
        // f(...)形式的调用
        if (!check(TokenType::RightParen)) {
            do {
                args->addExpression(parseExpression());
            } while (match(TokenType::Comma));
        }
        
        consume(TokenType::RightParen, "函数调用缺少右括号");
    } else if (match(TokenType::LeftBrace)) {
        // f {...} 形式的调用
        args->addExpression(parseTableConstructor());
        
    } else if (match(TokenType::String)) {
        // f "..." 形式的调用
        args->addExpression(make_ptr<LiteralExpr>(Object::Value::string(m_previous.lexeme)));
    } else {
        error("无效的函数调用语法");
        return nullptr;
    }
    
    return make_ptr<FunctionCallExpr>(callee, args);
}

Ptr<FunctionDeclStmt> Parser::parseFunctionDeclaration(bool isLocal) {
    // 解析函数名
    Vec<Str> nameComponents;
    bool isMethod = false;
    
    if (match(TokenType::Identifier)) {
        nameComponents.push_back(m_previous.lexeme);
        
        // 解析可能的表字段访问链
        while (match(TokenType::Dot)) {
            if (match(TokenType::Identifier)) {
                nameComponents.push_back(m_previous.lexeme);
            } else {
                error("函数名中表字段访问符号'.'后缺少标识符");
            }
        }
        
        // 检查是否是方法定义
        if (match(TokenType::Colon)) {
            if (match(TokenType::Identifier)) {
                nameComponents.push_back(m_previous.lexeme);
                isMethod = true;
            } else {
                error("函数方法名中':'后缺少标识符");
            }
        }
    } else {
        error("函数声明缺少函数名");
    }
    
    // 解析参数列表
    consume(TokenType::LeftParen, "函数定义缺少左括号");
    
    // 解析参数名列表
    Vec<Str> params;
    bool isVararg = false;
    
    if (isMethod) {
        // 方法有隐式self参数
        params.push_back("self");
    }
    
    if (!check(TokenType::RightParen)) {
        if (match(TokenType::Dots)) {
            isVararg = true;
        } else if (match(TokenType::Identifier)) {
            params.push_back(m_previous.lexeme);
            
            while (match(TokenType::Comma)) {
                if (match(TokenType::Dots)) {
                    isVararg = true;
                    break;
                } else if (match(TokenType::Identifier)) {
                    params.push_back(m_previous.lexeme);
                } else {
                    error("函数参数必须是标识符");
                }
            }
        }
    }
    
    consume(TokenType::RightParen, "函数参数列表缺少右括号");
    
    // 解析函数体
    auto body = parseBlock();
    
    consume(TokenType::End, "函数定义缺少'end'关键字");
    
    return make_ptr<FunctionDeclStmt>(nameComponents, isLocal, isMethod, params, isVararg, body);
}

Ptr<LocalVarDeclStmt> Parser::parseLocalVariableDeclaration() {
    // 解析变量名列表
    Vec<Str> names;
    
    if (match(TokenType::Identifier)) {
        names.push_back(m_previous.lexeme);
        
        while (match(TokenType::Comma)) {
            if (match(TokenType::Identifier)) {
                names.push_back(m_previous.lexeme);
            } else {
                error("局部变量声明中变量名必须是标识符");
            }
        }
    } else {
        error("局部变量声明缺少变量名");
    }
    
    // 解析可选的初始化表达式
    Vec<Ptr<Expression>> initializers;
    if (match(TokenType::Equal)) {
        initializers.push_back(parseExpression());
        
        while (match(TokenType::Comma)) {
            initializers.push_back(parseExpression());
        }
    }
    
    return make_ptr<LocalVarDeclStmt>(names, initializers);
}

Ptr<VariableExpr> Parser::parseVariable() {
    if (match(TokenType::Identifier)) {
        return make_ptr<VariableExpr>(m_previous.lexeme);
    }
    
    error("变量引用缺少标识符");
    return nullptr;
}

Ptr<NumericForStmt> Parser::parseNumericForStatement(const Str& varName) {
    // 解析起始值表达式
    auto start = parseExpression();
    
    consume(TokenType::Comma, "for循环起始值后缺少逗号");
    
    // 解析结束值表达式
    auto end = parseExpression();
    
    // 解析可选的步长表达式
    Ptr<Expression> step = nullptr;
    if (match(TokenType::Comma)) {
        step = parseExpression();
    } else {
        // 默认步长为1
        step = make_ptr<LiteralExpr>(Object::Value::number(1.0));
    }
    
    consume(TokenType::Do, "for循环条件后缺少'do'关键字");
    
    // 解析循环体
    auto body = parseBlock();
    
    consume(TokenType::End, "for语句后缺少'end'关键字");
    
    return make_ptr<NumericForStmt>(varName, start, end, step, body);
}

Ptr<GenericForStmt> Parser::parseGenericForStatement(const Vec<Str>& varNames) {
    // 解析迭代器表达式列表
    Vec<Ptr<Expression>> iterators;
    
    // 至少需要一个迭代器
    iterators.push_back(parseExpression());
    
    // 解析可选的额外迭代器
    while (match(TokenType::Comma)) {
        iterators.push_back(parseExpression());
    }
    
    consume(TokenType::Do, "for循环条件后缺少'do'关键字");
    
    // 解析循环体
    auto body = parseBlock();
    
    consume(TokenType::End, "for语句后缺少'end'关键字");
    
    return make_ptr<GenericForStmt>(varNames, iterators, body);
}

Ptr<WhileStmt> Parser::parseWhileStatement() {
    // 解析条件表达式
    auto condition = parseExpression();
    
    consume(TokenType::Do, "while语句缺少'do'关键字");
    
    // 解析循环体
    auto body = parseBlock();
    
    consume(TokenType::End, "while语句缺少'end'关键字");
    
    return make_ptr<WhileStmt>(condition, body);
}

Ptr<DoStmt> Parser::parseDoStatement() {
    // 解析do块
    auto body = parseBlock();
    
    consume(TokenType::End, "do语句缺少'end'关键字");
    
    return make_ptr<DoStmt>(body);
}

Ptr<RepeatStmt> Parser::parseRepeatStatement() {
    // 解析循环体
    auto body = parseBlock();
    
    consume(TokenType::Until, "repeat语句缺少'until'关键字");
    
    // 解析条件表达式
    auto condition = parseExpression();
    
    return make_ptr<RepeatStmt>(body, condition);
}

Ptr<BreakStmt> Parser::parseBreakStatement() {
    // break语句很简单，直接返回一个新实例
    return make_ptr<BreakStmt>();
}

Ptr<Statement> Parser::parseAssignmentOrCallStatement() {
    // 首先解析左侧表达式，可能是变量、表访问或字段访问
    auto expr = parsePrimary();
    
    // 如果是函数调用，则返回函数调用语句
    if (match(TokenType::LeftParen) || 
        match(TokenType::String) || 
        match(TokenType::LeftBrace)) {
        // 回退一个token，因为parseCall需要看到这些token
        m_current = m_previous;
        auto callExpr = parseCall(expr);
        return make_ptr<ExpressionStmt>(callExpr);
    }
    
    // 否则是赋值语句，收集左侧所有变量
    Vec<Ptr<Expression>> vars;
    vars.push_back(expr);
    
    while (match(TokenType::Comma)) {
        vars.push_back(parsePrimary());
    }
    
    // 解析等号和右侧表达式
    consume(TokenType::Equal, "赋值语句缺少'='号");
    
    // 解析右侧表达式列表
    Vec<Ptr<Expression>> values;
    values.push_back(parseExpression());
    
    while (match(TokenType::Comma)) {
        values.push_back(parseExpression());
    }
    
    return make_ptr<AssignmentStmt>(vars, values);
}

Ptr<ReturnStmt> Parser::parseReturnStatement() {
    Vec<Ptr<Expression>> values;
    
    // 如果不是语句结束标记，解析返回值表达式
    if (!check(TokenType::End) && 
        !check(TokenType::Else) && 
        !check(TokenType::Elseif) &&
        !check(TokenType::Until) && 
        !check(TokenType::Eof) && 
        !check(TokenType::Semicolon)) {
        
        values.push_back(parseExpression());
        
        while (match(TokenType::Comma)) {
            values.push_back(parseExpression());
        }
    }
    
    return make_ptr<ReturnStmt>(values);
}

Vec<Ptr<Expression>> Parser::parseExpressionList() {
    Vec<Ptr<Expression>> expressions;
    
    // 至少需要一个表达式
    expressions.push_back(parseExpression());
    
    // 解析可选的额外表达式
    while (match(TokenType::Comma)) {
        expressions.push_back(parseExpression());
    }
    
    return expressions;
}

Vec<Str> Parser::parseNameList() {
    Vec<Str> names;
    
    // 至少需要一个名称
    if (match(TokenType::Identifier)) {
        names.push_back(m_previous.lexeme);
        
        // 解析可选的额外名称
        while (match(TokenType::Comma)) {
            if (match(TokenType::Identifier)) {
                names.push_back(m_previous.lexeme);
            } else {
                error("名称列表中缺少标识符");
            }
        }
    } else {
        error("名称列表必须至少包含一个标识符");
    }
    
    return names;
}

Vec<Ptr<Expression>> Parser::parseArgumentList() {
    Vec<Ptr<Expression>> args;
    
    if (!check(TokenType::RightParen)) {
        // 解析第一个参数
        args.push_back(parseExpression());
        
        // 解析可选的额外参数
        while (match(TokenType::Comma)) {
            args.push_back(parseExpression());
        }
    }
    
    return args;
}

} // namespace Lua
