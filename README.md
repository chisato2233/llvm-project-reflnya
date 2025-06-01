# Class-Scoped Using Namespace for Clang

一个为C++添加class-scoped using namespace语法的实验性clang扩展，旨在解决头文件中namespace污染问题。

## 概述

这个项目扩展了C++语法，允许在class内部使用`using namespace`声明，将namespace的作用域限制在class内部，有效避免全局namespace污染。

## 动机

### 问题：Namespace污染
传统的`using namespace`在头文件中会造成全局污染：

```cpp
// header.h
using namespace std;  // 污染所有包含此头文件的代码

class MyClass {
    vector<int> data;  // 使用std::vector
};
```

### 解决方案：Class-Scoped Using Namespace
```cpp
// header.h  
class MyClass {
private:
    using namespace std;  // 只在MyClass内部生效
    
public:
    void process() {
        vector<int> temp;     // OK: 找到std::vector
        sort(temp.begin(), temp.end());  // OK: 找到std::sort
    }
};

// 外部不受影响
vector<int> global_var;  //错误：未声明的标识符
```

## 语言规范定义

### 语法 (Grammar)

#### class-member-specification:
```
member-declaration
member-declaration class-member-specification

member-declaration:
    decl-specifier-seq_opt member-declarator-list_opt ;
    function-definition
    using-declaration
    using-directive                    // 新增
    static_assert-declaration
    template-declaration
    alias-declaration
    empty-declaration
```

#### using-directive:
```
access-specifier:                      // 新增：访问控制修饰
    using namespace nested-name-specifier_opt namespace-name ;
```

### 语义规则 (Semantic Rules)

#### 3.4.6 class-scoped using-directive [namespace.classusing]

1. **作用域 (Scope)**
   - class-scoped using-directive的作用域 (scope) 是其直接出现的类作用域 (class scope)
   - 该using-directive在其出现点之后的所有类成员中生效
   - 作用域不延伸到嵌套类 (nested class)，除非该嵌套类没有自己的class-scoped using-directive

2. **查找语义 (Lookup Semantics)**  
   在类作用域 (class scope) 内进行无限定名称查找 (unqualified name lookup) 时：
   ```
   3.4.1.1 对于在类C中查找名称N：
   a) 首先在C中查找N的直接声明
   b) 然后在C的基类中查找N
   c) 然后应用C中的using-declaration
   d) 然后应用C中的class-scoped using-directive  // 新增步骤
   e) 最后应用外围作用域的using-directive
   ```

3. **访问控制 (Access Control)**
   - class-scoped using-directive受类成员访问控制规则约束
   - `private` using-directive：仅在声明类内部可访问，不被继承
   - `protected` using-directive：在声明类及其派生类中可访问 *(计划实现)*
   - `public` using-directive：在所有可访问该类的上下文中可访问 *(计划实现)*

4. **继承 (Inheritance)**
   ```
   11.2.x class-scoped using-directive继承规则：
   - private class-scoped using-directive不被任何派生类继承
   - protected class-scoped using-directive仅被public和protected继承的派生类继承
   - public class-scoped using-directive被所有派生类继承
   ```

5. **嵌套类访问 (Nested Class Access)**  
   ```
   11.12.x 嵌套类访问规则：
   嵌套类可以访问其直接或间接外围类 (enclosing class) 的所有class-scoped using-directive，
   无论其访问说明符为何，遵循现有的嵌套类访问语义。
   ```

### 名称查找详细规则 (Detailed Lookup Rules)

#### 3.4.1.x class-scoped using-directive查找

对于在类`C`的成员函数中出现的无限定标识符`id`：

1. **局部查找阶段**：
   - 在当前函数作用域中查找`id`
   - 在类`C`中查找`id`的直接声明
   - 在`C`的基类中查找`id`

2. **using-declaration阶段**：
   - 应用类`C`中所有可访问的using-declaration

3. **class-scoped using-directive阶段** *(新增)*：
    - 应用类`C`声明上下文中所有using-directive

4. **外围using-directive阶段**：
   - 应用外围命名空间作用域的using-directive

#### 歧义解决 (Ambiguity Resolution)

当多个class-scoped using-directive引入相同名称时：
```cpp
namespace A { int x; }
namespace B { int x; }

class C {
private:
    using namespace A;
    using namespace B;
public:
    void f() {
        int y = x;  // 错误：歧义，A::x 和 B::x 都可见
    }
};
```

### 实现定义行为 (Implementation-Defined Behavior)

1. **模板中的行为**：
   - class-scoped using-directive在模板实例化时的具体行为是实现定义的
   - 在模板特化中的表现未指定

2. **编译时计算**：
   - constexpr上下文中使用class-scoped using-directive引入的名称的行为是实现定义的

### 与现有标准的关系 (Relationship with Existing Standard)

#### 与7.3.4 using-directive的差异

| 特性 | 标准using-directive | class-scoped using-directive |
|------|-------------------|------------------------------|
| 作用域 | 出现点到最近外围命名空间 | 限制在声明类内部 |
| 继承性 | N/A | 受访问控制限制 |
| 访问控制 | 无 | private/protected/public |
| 查找优先级 | 最低 | 介于using-declaration和标准using-directive之间 |

#### 与11.9.3 using-declaration的关系

class-scoped using-directive是using-declaration的概念扩展：
- using-declaration引入特定名称到类作用域
- class-scoped using-directive引入整个命名空间到类作用域，但保持命名空间语义

### 格式良好性要求 (Well-formedness Requirements)

一个包含class-scoped using-directive的程序是格式良好的，当且仅当：

1. 被提名的命名空间在using-directive的出现点可见
2. 该using-directive不会导致类定义的递归依赖
3. 访问控制说明符在类定义中有效

### 诊断要求 (Diagnostic Requirements)

实现应当诊断以下情况：
- 在非类作用域中使用class-scoped using-directive语法
- 提名不存在的命名空间
- 在访问控制违规的上下文中使用class-scoped using-directive

## 实现状态

### ✅ 已完成
- [x] 基础语法解析 (`ParseDeclCXX.cpp`)
- [x] 语义分析框架 (`SemaDeclCXX.cpp`) 
- [x] AST节点创建和存储
- [x] 基础测试用例
- [x] Private访问控制语义

### 🚧 进行中
- [ ] 名称查找集成 (`SemaLookup.cpp`)
- [ ] 完整的错误处理和诊断

### 📋 计划中
- [ ] Protected和Public访问控制
- [ ] 性能优化
- [ ] 完整的测试覆盖
- [ ] 文档和示例

## 编译和测试

### 编译Clang
```bash
cd /path/to/llvm-project
mkdir build && cd build

# 配置（Debug版本，适合开发）
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug \
      -DLLVM_ENABLE_PROJECTS="clang" \
      -DLLVM_PARALLEL_LINK_JOBS=4 \
      ../llvm

# 编译
ninja clang
```

### 运行测试
```bash
# 运行class-scoped using namespace相关测试
./bin/llvm-lit clang/test/SemaCXX/class-scoped-using-namespace*.cpp -v

# 手动测试单个文件
echo 'namespace A{int x;} class B{private: using namespace A; public: void f(){int y=x;}};' | \
./bin/clang++ -fsyntax-only -xc++ -
```

### 测试文件
- `clang/test/SemaCXX/class-scoped-using-namespace.cpp` - 基础功能测试
- `clang/test/SemaCXX/class-scoped-using-namespace-access.cpp` - 访问控制测试  
- `clang/test/SemaCXX/class-scoped-using-namespace-errors.cpp` - 错误处理测试

## 使用示例

### 容器类库
```cpp
#include <vector>
#include <algorithm>

class Container {
private:
    using namespace std;
    
    vector<int> data;
    
public:
    void sort_data() {
        sort(data.begin(), data.end());  // 直接使用std::sort
    }
    
    void add(int value) {
        data.push_back(value);
    }
};
```

### 数学计算类
```cpp
namespace math_utils {
    double sin(double x);
    double cos(double x);
    const double PI = 3.14159;
}

class Calculator {
private:
    using namespace math_utils;
    
public:
    double calculate_area(double radius) {
        return PI * radius * radius;  // 直接使用math_utils::PI
    }
    
    double calculate_trig(double angle) {
        return sin(angle) + cos(angle);  // 直接使用math_utils函数
    }
};
```

## 技术细节

### 文件修改
- `clang/lib/Parse/ParseDeclCXX.cpp` - 添加class内using namespace解析
- `clang/lib/Sema/SemaDeclCXX.cpp` - 添加语义分析支持
- `clang/lib/Sema/SemaLookup.cpp` - 扩展名称查找机制
- `clang/include/clang/Sema/Sema.h` - 添加相关声明

### AST表示
使用标准的`UsingDirectiveDecl`节点，通过DeclContext存储，配合私有访问控制实现作用域限制。

### 名称查找
计划在`CppLookupName`中添加class-scoped using namespace查找阶段，优先级位于using declarations之后，普通using directives之前。

## 局限性

### 当前局限
- 只支持private访问控制
- 名称查找尚未完全集成
- 错误诊断需要完善

### 设计局限  
- 不支持在成员函数内使用
- 不支持条件编译中的using namespace
- Template特化中的行为未定义

## 贡献

这是一个实验性项目，欢迎贡献：

1. **测试用例**：添加更多边界情况测试
2. **错误修复**：修复现有实现中的bug
3. **功能扩展**：实现protected/public访问控制
4. **文档改进**：完善使用文档和示例

### 开发工作流
```bash
# 1. 修改代码
# 2. 编译测试
ninja clang
./bin/llvm-lit clang/test/SemaCXX/class-scoped-using-namespace*.cpp

# 3. 提交更改
git add .
git commit -m "[clang] Description of changes"
git push origin main
```

## 许可证

遵循LLVM项目的Apache 2.0 with LLVM Exception许可证。

## 联系

这是基于LLVM/Clang的实验性语言扩展项目。

---

*最后更新：2025年5月*
```