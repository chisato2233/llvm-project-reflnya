// RUN: %clang_cc1 -fsyntax-only -verify %s

namespace NonExistent; // expected-error {{expected namespace name}}

class ErrorTests {
private:
  // 测试using不存在的namespace
  using namespace NonExistent; // expected-error {{use of undeclared identifier 'NonExistent'}}
  
  // 测试using非namespace的标识符
  using namespace int; // expected-error {{expected namespace name}}
  
public:
  void test() {}
};

// 测试在全局作用域中错误使用class-scoped语法
// expected-error@+1 {{class-scoped using directive cannot be used in global scope}}
private:
  using namespace std; 