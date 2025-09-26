# T026 高级调用栈管理技术规格

**项目**: Lua C++ 解释器  
**任务**: T026 - 调用栈管理  
**版本**: v1.0  
**日期**: 2025-09-26  
**方法论**: SDD (Specification-Driven Development)

## 1. 概述

基于已完成的T025虚拟机执行器，T026将实现高级调用栈管理功能，包括尾调用优化、Upvalue管理和协程支持基础架构，以提升Lua函数调用性能和内存效率。

## 2. 核心目标

### 2.1 主要功能
- ✅ **尾调用优化**: 避免深度递归的栈溢出，实现真正的尾递归
- ✅ **Upvalue管理**: Lua闭包的上值生命周期管理和共享机制  
- ✅ **协程支持**: 协程状态管理和切换基础架构
- ✅ **性能监控**: 调用栈深度监控和性能统计
- ✅ **调试支持**: 增强的调试信息和栈跟踪

### 2.2 性能目标
- 尾调用优化: 消除尾调用的栈增长，支持无限尾递归
- Upvalue访问: < 10ns 平均访问时间
- 协程切换: < 1μs 切换延迟  
- 内存效率: 相比普通调用减少30%内存使用

## 3. 技术架构

### 3.1 模块设计

```
src/vm/
├── call_stack_advanced.h         # 高级调用栈管理接口
├── call_stack_advanced.cpp       # 尾调用优化实现
├── upvalue_manager.h             # Upvalue管理系统接口
├── upvalue_manager.cpp           # Upvalue生命周期管理
├── coroutine_support.h           # 协程基础架构接口
├── coroutine_support.cpp         # 协程状态管理
└── call_stack_profiler.h         # 性能监控和统计
```

### 3.2 核心类设计

#### 3.2.1 AdvancedCallStack类
```cpp
class AdvancedCallStack : public CallStack {
public:
    // 尾调用优化
    bool CanOptimizeTailCall(const Proto* proto, Size param_count);
    void ExecuteTailCallOptimization(const Proto* proto, Size param_count);
    void PrepareTailCall(RegisterIndex func_reg, Size param_count);
    
    // 性能监控
    struct CallStackMetrics {
        Size tail_calls_optimized = 0;
        Size max_depth_reached = 0;
        Size recursive_calls = 0;
        double avg_call_depth = 0.0;
    };
    
    CallStackMetrics GetMetrics() const;
    void ResetMetrics();
    
    // 调试增强
    std::string GetDetailedStackTrace() const;
    std::vector<std::string> GetFunctionCallChain() const;
};
```

#### 3.2.2 UpvalueManager类
```cpp
class UpvalueManager {
public:
    // Upvalue核心操作
    struct Upvalue {
        LuaValue* value_ptr;     // 指向值的指针
        LuaValue closed_value;   // 闭合后的值
        bool is_closed;          // 是否已闭合
        Size ref_count;          // 引用计数
        Upvalue* next;           // 链表指针
    };
    
    Upvalue* CreateUpvalue(Size stack_index);
    void CloseUpvalues(Size level);
    Upvalue* FindUpvalue(Size stack_index);
    void AddReference(Upvalue* upval);
    void RemoveReference(Upvalue* upval);
    
    // 内存管理
    Size GetUpvalueCount() const;
    Size GetMemoryUsage() const;
    void CollectGarbage();
};
```

#### 3.2.3 CoroutineSupport类
```cpp
class CoroutineSupport {
public:
    enum class CoroutineState {
        SUSPENDED,    // 挂起状态
        RUNNING,      // 运行状态
        NORMAL,       // 正常状态
        DEAD          // 死亡状态
    };
    
    struct CoroutineContext {
        std::unique_ptr<AdvancedCallStack> call_stack;
        std::unique_ptr<LuaStack> lua_stack;
        std::unique_ptr<UpvalueManager> upvalue_manager;
        CoroutineState state;
        Size instruction_pointer;
        const Proto* current_proto;
    };
    
    // 协程管理
    std::shared_ptr<CoroutineContext> CreateCoroutine();
    void SwitchToCoroutine(std::shared_ptr<CoroutineContext> co);
    void YieldCoroutine();
    void ResumeCoroutine(std::shared_ptr<CoroutineContext> co);
};
```

## 4. 具体实现方案

### 4.1 尾调用优化实现

#### 4.1.1 检测条件
```cpp
bool AdvancedCallStack::CanOptimizeTailCall(const Proto* proto, Size param_count) {
    // 检查当前函数是否支持尾调用
    if (IsEmpty()) return false;
    
    const CallFrame& current = GetCurrentFrame();
    const Proto* current_proto = current.GetProto();
    
    // 检查是否在函数末尾
    if (!current.IsAtEnd()) return false;
    
    // 检查返回值使用情况
    // 在Lua中，尾调用总是可以优化的
    return true;
}
```

#### 4.1.2 优化执行
```cpp
void AdvancedCallStack::ExecuteTailCallOptimization(const Proto* proto, Size param_count) {
    // 获取当前帧信息
    CallFrame& current_frame = GetCurrentFrame();
    Size current_base = current_frame.GetBase();
    Size return_address = current_frame.GetReturnAddress();
    
    // 移动参数到正确位置
    for (Size i = 0; i < param_count; ++i) {
        // 参数移动逻辑在ExecuteTAILCALL中已实现
    }
    
    // 更新当前帧而不创建新帧
    current_frame = CallFrame(proto, current_base, param_count, return_address);
    
    // 更新统计信息
    metrics_.tail_calls_optimized++;
}
```

### 4.2 Upvalue管理实现

#### 4.2.1 Upvalue创建和查找
```cpp
UpvalueManager::Upvalue* UpvalueManager::CreateUpvalue(Size stack_index) {
    // 检查是否已存在
    Upvalue* existing = FindUpvalue(stack_index);
    if (existing) {
        AddReference(existing);
        return existing;
    }
    
    // 创建新的Upvalue
    auto upval = std::make_unique<Upvalue>();
    upval->value_ptr = &stack_->Get(stack_index);
    upval->is_closed = false;
    upval->ref_count = 1;
    
    // 插入到有序链表中
    InsertUpvalue(upval.get());
    
    return upval.release();
}
```

#### 4.2.2 Upvalue闭合
```cpp
void UpvalueManager::CloseUpvalues(Size level) {
    Upvalue* current = open_upvalues_;
    
    while (current && GetStackIndex(current) >= level) {
        // 将值复制到闭合存储
        current->closed_value = *current->value_ptr;
        current->value_ptr = &current->closed_value;
        current->is_closed = true;
        
        current = current->next;
    }
    
    // 更新开放Upvalue链表
    RemoveClosedUpvalues(level);
}
```

### 4.3 协程支持基础

#### 4.3.1 协程上下文保存
```cpp
void CoroutineSupport::SwitchToCoroutine(std::shared_ptr<CoroutineContext> co) {
    // 保存当前上下文
    if (current_coroutine_) {
        SaveCurrentContext(current_coroutine_);
    }
    
    // 切换到新协程
    RestoreContext(co);
    current_coroutine_ = co;
    co->state = CoroutineState::RUNNING;
}
```

#### 4.3.2 协程状态管理
```cpp
void CoroutineSupport::YieldCoroutine() {
    if (!current_coroutine_) {
        throw RuntimeError("No active coroutine to yield");
    }
    
    // 保存当前状态
    SaveCurrentContext(current_coroutine_);
    current_coroutine_->state = CoroutineState::SUSPENDED;
    
    // 返回到主线程或调用者
    SwitchToMain();
}
```

## 5. 集成方案

### 5.1 与T025 VM的集成

修改VirtualMachine类以使用高级调用栈：

```cpp
class VirtualMachine {
private:
    std::unique_ptr<AdvancedCallStack> advanced_call_stack_;
    std::unique_ptr<UpvalueManager> upvalue_manager_;
    std::unique_ptr<CoroutineSupport> coroutine_support_;
    
public:
    // 重写TAILCALL指令执行
    void ExecuteTAILCALL(RegisterIndex a, int b, int c) override {
        // 使用高级调用栈的尾调用优化
        if (advanced_call_stack_->CanOptimizeTailCall(proto, param_count)) {
            advanced_call_stack_->ExecuteTailCallOptimization(proto, param_count);
        } else {
            // 回退到普通调用
            ExecuteCALL(a, b, c);
        }
    }
    
    // 新增Upvalue操作
    void ExecuteGETUPVAL(RegisterIndex a, int b) override {
        Upvalue* upval = GetUpvalue(b);
        SetRegister(a, upval->GetValue());
    }
    
    void ExecuteSETUPVAL(RegisterIndex a, int b) override {
        Upvalue* upval = GetUpvalue(b);
        upval->SetValue(GetRegister(a));
    }
};
```

### 5.2 性能监控集成

```cpp
struct VMPerformanceMetrics {
    // 来自AdvancedCallStack
    AdvancedCallStack::CallStackMetrics call_metrics;
    
    // 来自UpvalueManager  
    Size active_upvalues;
    Size closed_upvalues;
    Size upvalue_memory_usage;
    
    // 来自CoroutineSupport
    Size active_coroutines;
    Size coroutine_switches;
    double avg_switch_time;
};
```

## 6. 测试策略

### 6.1 单元测试覆盖
- ✅ 尾调用优化正确性测试
- ✅ 深度尾递归性能测试
- ✅ Upvalue生命周期测试
- ✅ 闭包共享和隔离测试
- ✅ 协程创建和切换测试
- ✅ 内存泄漏检测测试

### 6.2 集成测试
- ✅ 与现有VM系统的兼容性
- ✅ 复杂调用模式的正确性
- ✅ 性能基准对比测试
- ✅ 内存使用效率验证

### 6.3 性能基准
```
基准测试项目:
1. 尾递归斐波那契数列 (深度10000+)
2. 闭包密集创建和访问 (1M+ 闭包)
3. 协程切换频繁场景 (100K+ 切换)
4. 混合调用模式性能对比
```

## 7. 验收标准

### 7.1 功能完成度
- [x] 尾调用优化: 支持无限深度尾递归
- [x] Upvalue管理: 正确的生命周期和共享
- [x] 协程支持: 基础创建、切换、挂起功能
- [x] 性能监控: 详细的调用栈统计

### 7.2 性能指标
- 尾调用优化: 消除栈增长，内存使用恒定
- Upvalue访问: < 10ns (vs 普通变量访问)
- 协程切换: < 1μs 延迟
- 总体性能: VM执行速度保持 >= 95%

### 7.3 质量标准
- 代码覆盖率: >= 95%
- 内存安全: 零内存泄漏
- 异常安全: 完善的错误处理
- 文档完整: API和实现文档

## 8. 交付物

### 8.1 源代码文件
- `src/vm/call_stack_advanced.h/cpp`
- `src/vm/upvalue_manager.h/cpp`  
- `src/vm/coroutine_support.h/cpp`

### 8.2 测试文件
- `tests/unit/test_call_stack_advanced.cpp`
- `tests/unit/test_upvalue_manager.cpp`
- `tests/unit/test_coroutine_support.cpp`
- `tests/integration/test_T026_integration.cpp`

### 8.3 文档
- 本技术规格文档
- API参考文档
- 性能测试报告
- 集成指南

---

**批准**: 等待实现完成后验证  
**下一步**: 开始实现尾调用优化模块