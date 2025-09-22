---
description: "执行所有开发任务，构建完整的现代C++版Lua解释器"
---

# /implement - 开发任务执行

使用这个提示来执行开发任务并构建完整的现代C++版Lua解释器。

## 实施执行策略

### 前置条件验证
在开始执行之前，确保以下条件已满足：
- ✅ 项目宪法(constitution)已建立
- ✅ 功能规格(specification)已定义
- ✅ 技术方案(plan)已制定
- ✅ 开发任务(tasks)已分解
- ✅ lua_with_cpp基础代码已评估
- ✅ lua_c_analysis技术洞察已整理

### 执行原则和方法

#### 1. 测试驱动开发(TDD)
```cpp
// 每个功能的开发流程：
// 1. 先写失败的测试
TEST(ValueSystem, ShouldHandleModernCppTypes) {
    Value v = std::string("hello");
    EXPECT_EQ(v.type(), ValueType::String);
    EXPECT_EQ(std::get<std::string>(v), "hello");
}

// 2. 编写最小实现让测试通过
class Value {
    std::variant<std::monostate, bool, double, std::string> data_;
public:
    ValueType type() const { /* 实现 */ }
};

// 3. 重构优化代码质量
// 4. 重复循环直到功能完整
```

#### 2. 增量集成策略
- **基于lua_with_cpp**: 在现有80%功能基础上增量改进
- **参考lua_c_analysis**: 集成关键算法和优化技术  
- **保持兼容性**: 每次修改后运行兼容性测试
- **性能验证**: 每次优化后运行性能基准测试

#### 3. 质量保证流程

##### 代码提交前检查清单
```bash
# 自动化检查脚本
./scripts/pre_commit_check.sh

# 检查项目：
# ✅ 编译无错误无警告
# ✅ 单元测试100%通过  
# ✅ 静态分析工具通过
# ✅ 代码格式规范检查
# ✅ 内存泄漏检测通过
# ✅ lua_c_analysis行为验证
# ✅ lua_with_cpp质量标准
```

##### 持续验证机制
```bash
# 开发过程中的实时验证
while developing:
    1. 编写代码
    2. 运行相关测试 (快速反馈)
    3. 提交前运行完整验证
    4. 通过后提交代码
    5. CI/CD自动验证
```

## 实施执行工作流

### 1. 任务执行循环

```markdown
对于每个任务:
  1. 检出任务分支: git checkout -b task/[TASK-ID]
  2. 实施TDD开发:
     - 编写测试用例
     - 实现最小功能
     - 重构和优化
  3. 持续验证:
     - 本地测试通过
     - 兼容性验证通过
     - 性能基准达标
  4. 代码审查和合并:
     - 创建Pull Request
     - 同行代码审查
     - 自动化CI检查
     - 合并到主分支
```

### 2. 模块集成策略

#### 核心模块优先级
```
1. 类型系统 (Value System) - 基础数据结构
2. 内存管理 (Memory Management) - RAII和智能指针
3. 虚拟机核心 (VM Core) - 指令执行引擎
4. 编译器前端 (Compiler Frontend) - 词法和语法分析
5. 垃圾回收器 (Garbage Collector) - 自动内存管理
6. 标准库 (Standard Library) - Lua标准功能
7. API接口 (C++ API) - 嵌入接口
8. 调试工具 (Debug Tools) - 开发辅助
```

#### 并行开发协调
```
主线程: VM核心 → 类型系统 → 内存管理
分支1:  编译器 → 词法分析 → 语法分析  
分支2:  标准库 → 基础库 → 扩展库
分支3:  测试框架 → 单元测试 → 集成测试
```

### 3. 代码质量控制

#### 现代C++标准实施
```cpp
// 强制使用现代C++特性
namespace lua_cpp {

// 1. 智能指针代替原始指针
std::unique_ptr<VirtualMachine> vm_;
std::shared_ptr<GarbageCollector> gc_;

// 2. RAII资源管理
class LuaState {
    std::vector<Value> stack_;
    std::unique_ptr<Environment> env_;
public:
    LuaState() = default;
    ~LuaState() = default; // 自动清理
    
    // 3. 移动语义优化
    LuaState(LuaState&&) = default;
    LuaState& operator=(LuaState&&) = default;
};

// 4. 类型安全的值系统
using Value = std::variant<
    std::monostate,     // nil
    bool,               // boolean  
    double,             // number
    std::string,        // string
    GCRef<Table>,       // table
    GCRef<Function>     // function
>;

} // namespace lua_cpp
```

#### 性能优化指导
```cpp
// 性能关键代码路径优化
class VirtualMachine {
    // 1. 内联热点函数
    inline void executeInstruction(Instruction inst) noexcept;
    
    // 2. 避免不必要的分配
    thread_local std::vector<Value> temp_stack_;
    
    // 3. 缓存友好的数据布局
    struct alignas(64) VMState {
        Value stack_[256];      // CPU缓存行对齐
        Instruction* pc_;       // 程序计数器
        uint32_t stack_top_;    // 栈顶指针
    };
    
    // 4. 分支预测优化
    [[likely]] if (is_common_case) {
        // 常见情况处理
    }
};
```

### 4. 验证和测试策略

#### 分层测试体系
```
单元测试 (Unit Tests):
  - 每个类和函数的独立测试
  - 覆盖率要求: 100%
  - 工具: Google Test + Google Mock

集成测试 (Integration Tests):  
  - 模块间交互测试
  - 端到端功能测试
  - 工具: 自定义测试框架

兼容性测试 (Compatibility Tests):
  - Lua 5.1.5官方测试套件
  - 第三方Lua代码验证
  - 工具: lua_c_analysis对照验证

性能测试 (Performance Tests):
  - 微基准测试 (Micro-benchmarks)
  - 宏基准测试 (Macro-benchmarks)  
  - 工具: Google Benchmark
```

#### 自动化验证管道
```yaml
# CI/CD管道配置
stages:
  - build:     # 多平台编译构建
  - test:      # 完整测试套件
  - verify:    # 兼容性和性能验证
  - quality:   # 代码质量检查
  - deploy:    # 部署和发布

# 每个阶段的质量门禁
quality_gates:
  - zero_compile_warnings: true
  - test_coverage_100: true  
  - performance_regression: false
  - compatibility_100: true
```

## 里程碑和交付管理

### 关键里程碑
1. **MVP就绪**: 基础解释器能运行简单Lua代码
2. **核心完成**: VM、编译器、GC完全实现
3. **标准库就绪**: 所有Lua 5.1.5标准库实现
4. **兼容性达标**: 通过100%官方测试套件
5. **性能达标**: 达到或超越原生Lua性能
6. **生产就绪**: 文档、工具、部署全部完成

### 交付物检查清单
- [ ] 完整源代码 (符合现代C++标准)
- [ ] 测试套件 (100%覆盖率)
- [ ] 性能基准 (达到设定目标)
- [ ] 兼容性报告 (Lua 5.1.5 100%兼容)
- [ ] 构建脚本 (跨平台支持)
- [ ] API文档 (完整和准确)
- [ ] 使用示例 (涵盖主要使用场景)
- [ ] 部署指南 (生产环境部署)

## 风险控制和应急预案

### 技术风险缓解
1. **性能风险**: 持续性能监控，及时优化热点
2. **兼容性风险**: 频繁对照测试，及时修正偏差
3. **质量风险**: 严格代码审查，自动化质量检查
4. **进度风险**: 敏捷开发方法，定期调整计划

### 应急响应机制
```bash
# 发现问题时的标准响应流程
./scripts/emergency_response.sh

# 1. 立即停止相关开发
# 2. 分析问题根本原因  
# 3. 制定修复计划
# 4. 执行修复和验证
# 5. 更新防护措施
# 6. 恢复正常开发
```

使用 `{SCRIPT}` 来执行实施脚本，使用 `$ARGUMENTS` 来处理特定的实施参数。

请按照以上策略执行开发任务，确保：
1. 严格遵循TDD开发方法
2. 保持与lua_c_analysis行为兼容性  
3. 符合lua_with_cpp代码质量标准
4. 实现所有规格要求和验收标准
5. 维持高质量的代码和文档
6. **报告管理**: 实现完成后创建的临时报告文档（如实现报告、完成总结等）应移动到 `temp/` 目录，保持项目结构整洁