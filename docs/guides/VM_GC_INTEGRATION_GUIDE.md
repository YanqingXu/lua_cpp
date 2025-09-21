# VM集成指南 - 垃圾收集器

## 📋 集成概述

本指南详细说明如何将独立的标记-清扫垃圾收集器集成到VM系统中。

## 🔧 集成前提条件

### 类型系统统一
在集成前，需要解决以下类型定义冲突：

#### 1. LuaType枚举统一
```cpp
// 需要在一个中央头文件中统一定义
enum class LuaType : uint8_t {
    NIL = 0,
    BOOLEAN,
    NUMBER,
    STRING,
    TABLE,
    FUNCTION,
    USERDATA,
    THREAD
};
```

#### 2. 错误类型系统
```cpp
// 统一使用ErrorCode而不是ErrorType
enum class ErrorCode {
    Success = 0,
    RuntimeError,
    MemoryError,
    SyntaxError,
    TypeError
};
```

#### 3. 操作码定义
```cpp
// 确保OpCode在所有模块中一致定义
enum class OpCode : uint8_t {
    OP_MOVE = 0,
    OP_LOADK,
    OP_LOADBOOL,
    // ... 其他操作码
};
```

#### 4. 寄存器索引类型
```cpp
using RegisterIndex = uint16_t;  // 在全局范围定义
```

## 🏗️ 核心集成接口

### GC管理的LuaValue

```cpp
class LuaValue {
private:
    LuaType type_;
    union {
        bool boolean_;
        double number_;
        GCObject* object_;  // 字符串、表、函数等
    } value_;

public:
    // GC相关方法
    bool IsGCObject() const {
        return type_ == LuaType::STRING || 
               type_ == LuaType::TABLE || 
               type_ == LuaType::FUNCTION;
    }
    
    GCObject* GetGCObject() const {
        return IsGCObject() ? value_.object_ : nullptr;
    }
    
    void SetGCObject(GCObject* obj, LuaType type) {
        type_ = type;
        value_.object_ = obj;
    }
};
```

### VM根对象扫描接口

```cpp
class VirtualMachine {
public:
    // 为GC提供根对象扫描
    void ScanRoots(std::function<void(GCObject*)> visitor) {
        // 1. 扫描执行栈
        ScanStack(visitor);
        
        // 2. 扫描调用帧
        ScanCallFrames(visitor);
        
        // 3. 扫描全局变量
        ScanGlobals(visitor);
        
        // 4. 扫描注册表
        ScanRegistry(visitor);
    }

private:
    void ScanStack(std::function<void(GCObject*)> visitor) {
        for (size_t i = 0; i < stack_top_; ++i) {
            if (stack_[i].IsGCObject()) {
                visitor(stack_[i].GetGCObject());
            }
        }
    }
    
    void ScanCallFrames(std::function<void(GCObject*)> visitor) {
        for (const auto& frame : call_frames_) {
            if (frame.function && frame.function->IsGCObject()) {
                visitor(frame.function->GetGCObject());
            }
        }
    }
    
    void ScanGlobals(std::function<void(GCObject*)> visitor) {
        // 扫描全局表中的所有GC对象
        if (globals_.IsGCObject()) {
            visitor(globals_.GetGCObject());
        }
    }
    
    void ScanRegistry(std::function<void(GCObject*)> visitor) {
        // 扫描注册表中的所有GC对象
        if (registry_.IsGCObject()) {
            visitor(registry_.GetGCObject());
        }
    }
};
```

## 🔗 集成的GC系统

### 继承独立GC实现

```cpp
class IntegratedGC : public StandaloneGC {
private:
    VirtualMachine* vm_;

public:
    IntegratedGC(VirtualMachine* vm, Size initial_threshold = 1024) 
        : StandaloneGC(initial_threshold), vm_(vm) {}

protected:
    // 重写根对象扫描
    void ScanRoots() override {
        if (!vm_) return;
        
        vm_->ScanRoots([this](GCObject* obj) {
            if (obj && IsWhite(obj)) {
                MarkGray(obj);
            }
        });
    }
};
```

### VM中集成GC

```cpp
class VirtualMachine {
private:
    std::unique_ptr<IntegratedGC> gc_;
    
public:
    VirtualMachine() {
        gc_ = std::make_unique<IntegratedGC>(this);
    }
    
    // 创建GC管理的对象
    template<typename T, typename... Args>
    T* CreateGCObject(Args&&... args) {
        return gc_->CreateObject<T>(std::forward<Args>(args)...);
    }
    
    // 在适当时机触发GC
    void CheckGC() {
        if (gc_->ShouldCollect()) {
            gc_->Collect();
        }
    }
    
    // 增量GC步骤
    void PerformGCStep() {
        gc_->PerformIncrementalStep();
    }
};
```

## 📝 Lua对象GC适配

### 字符串对象

```cpp
class LuaString : public GCObject {
private:
    std::string data_;
    
public:
    LuaString(const std::string& str) : data_(str) {}
    
    const std::string& GetData() const { return data_; }
    
    // GC接口实现
    void MarkReferences(std::function<void(GCObject*)> marker) override {
        // 字符串没有引用其他对象
    }
    
    Size GetSize() const override {
        return sizeof(*this) + data_.capacity();
    }
};
```

### 表对象

```cpp
class LuaTable : public GCObject {
private:
    std::unordered_map<LuaValue, LuaValue> map_;
    
public:
    // GC接口实现
    void MarkReferences(std::function<void(GCObject*)> marker) override {
        for (const auto& [key, value] : map_) {
            if (key.IsGCObject()) {
                marker(key.GetGCObject());
            }
            if (value.IsGCObject()) {
                marker(value.GetGCObject());
            }
        }
    }
    
    Size GetSize() const override {
        return sizeof(*this) + map_.size() * sizeof(std::pair<LuaValue, LuaValue>);
    }
};
```

### 函数对象

```cpp
class LuaFunction : public GCObject {
private:
    std::vector<LuaValue> upvalues_;
    std::vector<uint8_t> bytecode_;
    
public:
    // GC接口实现
    void MarkReferences(std::function<void(GCObject*)> marker) override {
        for (const auto& upval : upvalues_) {
            if (upval.IsGCObject()) {
                marker(upval.GetGCObject());
            }
        }
    }
    
    Size GetSize() const override {
        return sizeof(*this) + 
               upvalues_.capacity() * sizeof(LuaValue) +
               bytecode_.capacity();
    }
};
```

## ⚡ 性能优化建议

### 1. GC触发策略

```cpp
class VirtualMachine {
private:
    size_t allocations_since_gc_ = 0;
    
public:
    void OnObjectAllocated() {
        allocations_since_gc_++;
        
        // 每1000次分配检查一次
        if (allocations_since_gc_ % 1000 == 0) {
            CheckGC();
        }
    }
};
```

### 2. 增量GC集成

```cpp
// 在字节码执行循环中
void VirtualMachine::ExecuteBytecode() {
    while (pc_ < bytecode_.size()) {
        // 每执行N条指令进行一次增量GC步骤
        if (++gc_step_counter_ >= GC_STEP_INTERVAL) {
            PerformGCStep();
            gc_step_counter_ = 0;
        }
        
        // 执行当前指令
        ExecuteInstruction();
    }
}
```

### 3. 写屏障实现

```cpp
template<typename T>
void VirtualMachine::WriteBarrier(GCObject* parent, T* child) {
    // 如果父对象是黑色，子对象是白色，需要标记子对象
    if (gc_->IsBlack(parent) && gc_->IsWhite(child)) {
        gc_->MarkGray(child);
    }
}
```

## 🧪 集成测试

### 基础功能测试

```cpp
void TestVMGCIntegration() {
    VirtualMachine vm;
    
    // 测试字符串创建和回收
    auto* str1 = vm.CreateGCObject<LuaString>("hello");
    auto* str2 = vm.CreateGCObject<LuaString>("world");
    
    // 压入栈（设置为根对象）
    vm.PushValue(LuaValue(str1, LuaType::STRING));
    
    // 触发GC - str2应该被回收，str1应该保留
    vm.GetGC()->Collect();
    
    assert(vm.GetGC()->IsAlive(str1));
    // str2已被回收，无法直接检查
}
```

### 复杂对象图测试

```cpp
void TestComplexObjectGraph() {
    VirtualMachine vm;
    
    // 创建表和字符串的复杂关系
    auto* table = vm.CreateGCObject<LuaTable>();
    auto* key = vm.CreateGCObject<LuaString>("key");
    auto* value = vm.CreateGCObject<LuaString>("value");
    
    table->Set(LuaValue(key, LuaType::STRING), 
               LuaValue(value, LuaType::STRING));
    
    // 只保护表对象
    vm.PushValue(LuaValue(table, LuaType::TABLE));
    
    vm.GetGC()->Collect();
    
    // 表、键、值都应该存活
    assert(vm.GetGC()->IsAlive(table));
    assert(vm.GetGC()->IsAlive(key));
    assert(vm.GetGC()->IsAlive(value));
}
```

## 📊 集成检查清单

### 必须完成的任务

- [ ] **类型系统统一**
  - [ ] LuaType枚举统一定义
  - [ ] ErrorCode错误类型统一
  - [ ] OpCode操作码一致性
  - [ ] RegisterIndex类型定义

- [ ] **VM接口实现**
  - [ ] LuaValue GC对象支持
  - [ ] VM根对象扫描接口
  - [ ] GC触发时机集成
  - [ ] 增量GC步骤集成

- [ ] **Lua对象适配**
  - [ ] LuaString GC接口
  - [ ] LuaTable GC接口  
  - [ ] LuaFunction GC接口
  - [ ] 其他用户数据类型

- [ ] **性能优化**
  - [ ] 写屏障实现
  - [ ] GC触发策略优化
  - [ ] 内存使用监控
  - [ ] 增量收集调优

- [ ] **测试验证**
  - [ ] 基础GC功能测试
  - [ ] 复杂对象图测试
  - [ ] 性能基准测试
  - [ ] 内存泄漏检测

### 可选增强功能

- [ ] **分代收集**
- [ ] **并发收集**
- [ ] **内存压缩**
- [ ] **GC统计监控**
- [ ] **调试支持**

## 🚀 集成步骤

### 阶段1: 基础集成
1. 解决类型定义冲突
2. 实现基础VM-GC接口
3. 适配核心Lua对象类型

### 阶段2: 功能完善
1. 实现写屏障
2. 优化GC触发策略
3. 添加增量收集支持

### 阶段3: 性能优化
1. 调优收集算法参数
2. 实现内存使用监控
3. 添加性能分析工具

### 阶段4: 测试验证
1. 全面的功能测试
2. 性能基准测试
3. 内存安全验证

---

**注意**: 这个集成指南基于已经验证的独立GC实现。集成过程中如遇到具体技术问题，请参考独立实现的成功经验。