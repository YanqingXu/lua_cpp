/**
 * @file example_contract_setup.cpp
 * @brief 契约测试设置示例
 * @description 展示如何为新模块建立契约测试
 */

#include <catch2/catch_test_macros.hpp>

// 第1步：定义抽象接口（无需实现）
namespace lua_cpp {

class ILexer {
public:
    virtual ~ILexer() = default;
    virtual Token NextToken() = 0;
    virtual bool IsAtEnd() const = 0;
    virtual TokenPosition GetPosition() const = 0;
};

class IParser {
public:
    virtual ~IParser() = default;
    virtual std::unique_ptr<ASTNode> ParseExpression() = 0;
    virtual std::unique_ptr<ASTNode> ParseStatement() = 0;
    virtual bool HasErrors() const = 0;
};

class IGarbageCollector {
public:
    virtual ~IGarbageCollector() = default;
    virtual void MarkObject(GCObject* obj) = 0;
    virtual void SweepPhase() = 0;
    virtual size_t GetHeapSize() const = 0;
};

} // namespace lua_cpp

// 第2步：编写契约测试（定义期望行为）
TEST_CASE("新模块契约测试模板", "[module][contract]") {
    SECTION("基础功能契约") {
        // 定义期望的接口行为
        // 这些测试在实现之前会失败，这是正常的
        
        REQUIRE(true); // 占位符，防止编译错误
        
        /* 将来的实现：
        auto lexer = CreateLexer("test input");
        auto token = lexer->NextToken();
        REQUIRE(token.GetType() == TokenType::Expected);
        */
    }
    
    SECTION("错误处理契约") {
        // 定义错误情况的期望行为
        REQUIRE(true);
    }
    
    SECTION("性能契约") {
        // 定义性能要求
        REQUIRE(true);
    }
}

// 第3步：Mock工厂（用于测试隔离）
namespace lua_cpp::test {

class MockLexer : public ILexer {
    // Mock实现，用于测试其他模块
};

class MockParser : public IParser {
    // Mock实现，用于测试依赖Parser的模块
};

// 工厂函数
std::unique_ptr<ILexer> CreateMockLexer() {
    return std::make_unique<MockLexer>();
}

} // namespace lua_cpp::test