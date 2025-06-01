# Class-Scoped Using Namespace for Clang

一个为C++添加class-scoped using namespace语法的实验性clang扩展，旨在解决头文件中namespace污染问题。

## 概述

这个项目扩展了C++语法，允许在class内部使用`using namespace`声明，将namespace的作用域限制在class内部，有效避免全局namespace污染。

## 动机

### 问题：Namespace污染
传统的`using namespace`在头文件中会造成全局污染：

```cpp
// header.h
using namespace std;  // 💥 污染所有包含此头文件的代码

class MyClass {
    vector<int> data;  // 使用std::vector
};
```

### 解决方案：Class-Scoped Using Namespace
```cpp
// header.h  
class MyClass {
private:
    using namespace std;  // ✅ 只在MyClass内部生效
    
public:
    void process() {
        vector<int> temp;     // OK: 找到std::vector
        sort(temp.begin(), temp.end());  // OK: 找到std::sort
    }
};

// 外部不受影响
vector<int> global_var;  // ❌ 错误：未声明的标识符
```

## 语法特性

### 基本语法
```cpp
class MyClass {
private:
    using namespace some_namespace;  // 私有using namespace

public:
    void func() {
        // 可以使用some_namespace中的符号
    }
};
```

### 访问控制语义
- **Private**: 只在声明的class内可见，不被继承 *(当前实现)*
- **Protected**: 被protected继承的派生类可见 *(计划中)*  
- **Public**: 所有派生类可见 *(计划中)*

### 嵌套类语义
```cpp
class Outer {
private:
    using namespace A;
    
    class Inner {
    public:
        void test() {
            // ✅ 可以访问A中的符号（嵌套类可以访问外部类private成员）
        }
    };
};
```

### 继承语义
```cpp
class Base {
private:
    using namespace A;
public:
    void test() { /* 可以使用A中的符号 */ }
};

class Derived : public Base {
public:
    void test() {
        // ❌ 不能访问A中的符号（private using不被继承）
    }
};
```

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

*最后更新：2024年12月*
