// RUN: %clang_cc1 -fsyntax-only -verify %s

namespace Utils {
  int helper_value = 42;
  void helper_func();
}

// 测试不同访问级别（当前只支持private）
class AccessTest {
private:
  using namespace Utils;
  
  // TODO: 未来支持protected和public时取消这些错误期望
  // protected:
  //   using namespace Utils; // 将来应该支持
  
  // public:
  //   using namespace Utils; // 将来应该支持
  
public:
  void test() {
    int x = helper_value;  // OK: private using在同一个类中可访问
    helper_func();         // OK
  }
};

class AccessTestDerived : public AccessTest {
public:
  void test() {
    // expected-error@+2 {{use of undeclared identifier 'helper_value'}}
    // expected-note@4 {{'Utils::helper_value' declared here}}
    int x = helper_value;  // 应该失败：无法访问基类的private using
    
    // expected-error@+2 {{use of undeclared identifier 'helper_func'}}  
    // expected-note@5 {{'Utils::helper_func' declared here}}
    helper_func();         // 应该失败
  }
}; 