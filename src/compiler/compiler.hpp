#pragma once

#include "types.hpp"
#include "ast.hpp"

#include "compiler/parser.hpp"
#include "vm/function_proto.hpp"
#include "object/value.hpp"

#include <memory>


namespace Lua {

/**
 * @brief 编译器类，负责将AST转换为字节码
 */
class Compiler {
public:
    /**
     * 编译代码
     * @param ast 抽象语法树
     * @param source 源代码（用于错误报告）
     * @return 编译后的函数原型
     */
    Ptr<FunctionProto> compile(Ptr<Block> ast, const Str& source);

	/**
	 * 获取当前编译状态的源代码
	 * @return 源代码字符串
	 */
	const Str& getSource() const {
		return m_source;
	}

    /**
	* 设置当前编译状态的源代码
	* @param source 新的源代码字符串
    */
	void setSource(const Str& source) {
		m_source = source;
	}

private:
    struct CompileState {
        Ptr<FunctionProto> proto;       // 当前函数原型
        HashMap<Str, i32> locals;           // 局部变量名到索引的映射
        Vec<Upvalue> upvalues;          // upvalue表
        i32 scopeDepth = 0;                 // 当前作用域深度
        i32 localCount = 0;                 // 局部变量数量
        i32 stackSize = 0;                  // 当前栈大小
        CompileState* enclosing = nullptr;          // 外层编译状态
    };

    CompileState* m_current = nullptr;              // 当前编译状态
    Str m_source;                           // 源代码

    // 作用域管理
    void beginScope();
    void endScope();

    // 局部变量管理
    i32 addLocal(const Str& name);
    i32 resolveLocal(CompileState* state, const Str& name);
    i32 resolveUpvalue(CompileState* state, const Str& name);

    // 表达式编译
    void compileExpression(Ptr<Expression> expr, i32 reg);
    void compileBinaryExpr(Ptr<BinaryExpr> expr, i32 reg);
    void compileUnaryExpr(Ptr<UnaryExpr> expr, i32 reg);
    void compileLiteralExpr(Ptr<LiteralExpr> expr, i32 reg);
    void compileVariableExpr(Ptr<VariableExpr> expr, i32 reg);
    void compileTableAccessExpr(Ptr<TableAccessExpr> expr, i32 reg);
    void compileFieldAccessExpr(Ptr<FieldAccessExpr> expr, i32 reg);
    void compileFunctionCallExpr(Ptr<FunctionCallExpr> expr, i32 reg);
    void compileTableConstructorExpr(Ptr<TableConstructorExpr> expr, i32 reg);
    void compileFunctionDefExpr(Ptr<FunctionDefExpr> expr, i32 reg);

    // 语句编译
    void compileStatement(Ptr<Statement> stmt);
    void compileBlock(Ptr<Block> block);
    void compileAssignmentStmt(Ptr<AssignmentStmt> stmt);
    void compileLocalVarDeclStmt(Ptr<LocalVarDeclStmt> stmt);
    void compileFunctionCallStmt(Ptr<FunctionCallStmt> stmt);
    void compileIfStmt(Ptr<IfStmt> stmt);
    void compileWhileStmt(Ptr<WhileStmt> stmt);
    void compileDoStmt(Ptr<DoStmt> stmt);
    void compileForStmt(Ptr<NumericForStmt> stmt);
    void compileGenericForStmt(Ptr<GenericForStmt> stmt);
    void compileRepeatStmt(Ptr<RepeatStmt> stmt);
    void compileFunctionDeclStmt(Ptr<FunctionDeclStmt> stmt);
    void compileReturnStmt(Ptr<ReturnStmt> stmt);
    void compileBreakStmt(Ptr<BreakStmt> stmt);

    // 指令生成
    usize emitInstruction(const Instruction& instruction, i32 line);
    usize emitABC(OpCode op, u8 a, u8 b, u8 c, i32 line);
    usize emitABx(OpCode op, u8 a, u16 bx, i32 line);
    usize emitASBx(OpCode op, u8 a, i16 sbx, i32 line);
    usize emitAx(OpCode op, u32 ax, i32 line);

    // 跳转指令处理
    usize emitJump(OpCode op, i32 line);
    void patchJump(usize jumpInstr, usize target);

    // 常量处理
    i32 addConstant(const Object::Value& value);
    i32 addStringConstant(const Str& str);
};

} // namespace Lua
