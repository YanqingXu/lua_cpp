# T028 协程标准库开发文档索引

**任务**: T028 - 协程标准库支持（基于C++20）  
**状态**: 📋 准备就绪，可以开始开发  
**创建日期**: 2025-10-11  
**预计完成**: 2025-10-13  

---

## 📚 文档清单

本目录包含T028任务的完整开发文档，共5个核心文档：

| # | 文档 | 用途 | 阅读时间 | 优先级 |
|---|------|------|----------|--------|
| 1️⃣ | **T028_MASTER_GUIDE.md** | 📖 总览文档（从这里开始） | 10分钟 | ⭐⭐⭐ |
| 2️⃣ | **T028_QUICK_START.md** | 🚀 快速启动指南 | 15分钟 | ⭐⭐⭐ |
| 3️⃣ | **specs/T028_COROUTINE_STDLIB_PLAN.md** | 📐 详细技术计划 | 30分钟 | ⭐⭐ |
| 4️⃣ | **docs/CPP20_COROUTINE_REFERENCE.md** | 🔧 C++20协程技术参考 | 20分钟 | ⭐⭐ |
| 5️⃣ | **PROJECT_DASHBOARD.md** | 📊 项目进度仪表板 | 5分钟 | ⭐ |

---

## 🎯 快速导航

### 场景1: 第一次接触T028任务
```
阅读顺序：
1. T028_MASTER_GUIDE.md (10分钟) - 了解全貌
2. T028_QUICK_START.md (15分钟) - 查看实施清单
3. 开始Phase 1开发
```

### 场景2: 需要技术细节
```
阅读顺序：
1. specs/T028_COROUTINE_STDLIB_PLAN.md (30分钟) - 完整技术方案
2. docs/CPP20_COROUTINE_REFERENCE.md (20分钟) - C++20技术参考
```

### 场景3: 遇到具体问题
```
查找文档：
- C++20协程问题 → CPP20_COROUTINE_REFERENCE.md
- 实施步骤问题 → T028_QUICK_START.md
- 架构设计问题 → T028_COROUTINE_STDLIB_PLAN.md
- 项目进度查询 → PROJECT_DASHBOARD.md
```

---

## 📋 文档详情

### 1. T028_MASTER_GUIDE.md - 总览文档
**内容**:
- ✅ 所有文档的导航索引
- ✅ 核心技术摘要
- ✅ 6阶段开发路线图
- ✅ 关键代码片段
- ✅ 学习路径建议
- ✅ FAQ常见问题

**适合**:
- 项目经理 - 了解任务全貌
- 新开发者 - 快速理解任务
- 老开发者 - 查找具体信息

### 2. T028_QUICK_START.md - 快速启动
**内容**:
- ⚡ 15分钟快速理解
- ⚡ 文件结构清单
- ⚡ Phase 1-6 Checklist
- ⚡ 关键代码模板
- ⚡ 测试模板
- ⚡ FAQ快速解答

**适合**:
- 开发者 - 立即开始编码
- 查找 - 具体实施步骤
- 参考 - 代码模板

### 3. specs/T028_COROUTINE_STDLIB_PLAN.md - 详细计划
**内容**:
- 📐 完整技术方案（1500+行）
- 📐 架构设计和类图
- 📐 6个阶段详细分解
- 📐 性能优化策略
- 📐 质量保证体系
- 📐 100+代码示例

**适合**:
- 架构师 - 理解技术方案
- 开发者 - 查看实现细节
- 测试 - 了解验收标准

### 4. docs/CPP20_COROUTINE_REFERENCE.md - C++20参考
**内容**:
- 🔧 C++20协程核心概念
- 🔧 Promise Type完整解析
- 🔧 Awaiter模式详解
- 🔧 高级技巧和最佳实践
- 🔧 性能优化技术
- 🔧 调试技巧

**适合**:
- C++20新手 - 学习协程特性
- 有经验者 - 查找高级技巧
- 性能优化 - 优化参考

### 5. PROJECT_DASHBOARD.md - 项目仪表板
**内容**:
- 📊 T028当前状态
- 📊 项目整体进度
- 📊 已完成任务列表
- 📊 技术栈和成就

**适合**:
- 管理者 - 查看进度
- 开发者 - 了解上下文

---

## 🚀 立即开始

### Step 1: 理解任务（10分钟）
```bash
# 阅读总览文档
cat T028_MASTER_GUIDE.md
```

### Step 2: 查看清单（15分钟）
```bash
# 查看快速启动
cat T028_QUICK_START.md
```

### Step 3: 开始开发
```bash
# 创建功能分支
git checkout -b feature/T028-coroutine-stdlib

# 创建文件
touch src/stdlib/coroutine_lib.h
touch src/stdlib/coroutine_lib.cpp

# 开始编码
code src/stdlib/coroutine_lib.h
```

---

## 📊 任务概览

### 核心目标
实现Lua 5.1.5 `coroutine.*` 标准库，基于C++20协程特性

### 需实现的API
1. `coroutine.create(f)` - 创建协程
2. `coroutine.resume(co, ...)` - 恢复执行
3. `coroutine.yield(...)` - 挂起当前协程
4. `coroutine.status(co)` - 查询状态
5. `coroutine.running()` - 获取当前协程
6. `coroutine.wrap(f)` - 创建协程包装器

### 性能目标
- 协程创建: < 5μs
- Resume/Yield: < 100ns
- 内存开销: < 1KB per coroutine

### 质量目标
- 测试覆盖率: ≥ 95%
- Lua兼容性: 100%
- 编译警告: 0
- 内存泄漏: 0

### 预计工期
- Phase 1-2: Day 1 (10-14小时)
- Phase 3-4: Day 2 (9-12小时)
- Phase 5-6: Day 3 (4-6小时)
- **总计**: 2-3天

---

## 🎓 技术亮点

### C++20协程优势
- ✨ **零成本抽象** - 编译期优化
- ✨ **类型安全** - 编译期检查
- ✨ **现代语法** - `co_await`/`co_yield`/`co_return`
- ✨ **标准化** - `<coroutine>`标准头文件

### 架构创新
- 🏗️ **清晰分层** - Lua API → C++20协程 → 标准库
- 🏗️ **模块化** - 独立的协程库模块
- 🏗️ **可扩展** - 预留协程池化等优化空间

### 质量保证
- 🧪 **TDD驱动** - 测试先行
- 🧪 **全面测试** - 单元+集成+性能
- 🧪 **双重验证** - lua_c_analysis + lua_with_cpp

---

## ✅ 验收标准

### 功能完整性 ✓
- [ ] 6个Lua API全部实现
- [ ] Lua 5.1.5完全兼容

### 质量保证 ✓
- [ ] 测试覆盖率 ≥ 95%
- [ ] 零编译警告
- [ ] 零内存泄漏

### 性能达标 ✓
- [ ] 创建 < 5μs
- [ ] Resume/Yield < 100ns
- [ ] 内存 < 1KB

### 文档完善 ✓
- [ ] API文档
- [ ] 完成报告
- [ ] 项目文档更新

---

## 📞 获取帮助

### 查找文档
- **技术问题**: `docs/CPP20_COROUTINE_REFERENCE.md`
- **实施步骤**: `T028_QUICK_START.md`
- **设计方案**: `specs/T028_COROUTINE_STDLIB_PLAN.md`
- **FAQ**: `T028_MASTER_GUIDE.md` 第7章

### 外部资源
- [C++20协程 - cppreference](https://en.cppreference.com/w/cpp/language/coroutines)
- [Lewis Baker博客](https://lewissbaker.github.io/)
- [Lua 5.1.5手册](https://www.lua.org/manual/5.1/manual.html#2.11)

---

## 🎯 开发者快速入口

### 我是C++20新手
```
1. 阅读: CPP20_COROUTINE_REFERENCE.md (20分钟)
2. 学习: Lewis Baker博客 (1小时)
3. 实践: 简单示例 (2小时)
4. 开始: T028开发
```

### 我有C++20经验
```
1. 阅读: T028_COROUTINE_STDLIB_PLAN.md (30分钟)
2. 查看: T028_QUICK_START.md (10分钟)
3. 开始: Phase 1开发
```

### 我只想快速开始
```
1. 查看: T028_QUICK_START.md Phase 1 Checklist
2. 创建: 文件结构
3. 编码: 从LuaCoroutine类开始
```

---

## 📈 文档统计

| 文档 | 行数 | 章节数 | 代码示例 |
|------|------|--------|----------|
| T028_MASTER_GUIDE.md | 600+ | 10 | 15+ |
| T028_QUICK_START.md | 800+ | 9 | 20+ |
| T028_COROUTINE_STDLIB_PLAN.md | 1500+ | 58 | 100+ |
| CPP20_COROUTINE_REFERENCE.md | 1200+ | 9 | 80+ |
| **总计** | **4100+** | **86** | **215+** |

---

## 🎉 准备就绪！

所有文档已准备完毕，T028协程标准库开发可以开始了！

**下一步**: 阅读 `T028_MASTER_GUIDE.md` → 查看 `T028_QUICK_START.md` → 开始 Phase 1

**祝开发顺利！🚀**

---

**文档创建日期**: 2025-10-11  
**文档维护者**: lua_cpp项目团队  
**文档版本**: 1.0
