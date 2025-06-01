// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++17 %s

namespace A {
  int value = 100;
  void func();
  struct Helper {};
}

namespace B {
  int value = 200;
  void func();
  struct Helper {};
}

// 测试基本的private class-scoped using namespace
class TestClassPrivate {
private:
  using namespace A;
  
public:
  void test() {
    int x = value;        // OK: 应该找到A::value
    func();              // OK: 应该找到A::func
    Helper h;            // OK: 应该找到A::Helper
  }
};

// 测试private using不被派生类继承
class DerivedFromPrivate : public TestClassPrivate {
private:
  using namespace B;
  
public:
  void testDerived() {
    int x = value;        // OK: 应该找到B::value，不是A::value
    func();              // OK: 应该找到B::func，不是A::func
    Helper h;            // OK: 应该找到B::Helper，不是A::Helper
  }
};

// 测试在派生类中访问基类的private using应该失败
class DerivedAccessTest : public TestClassPrivate {
public:
  void testAccess() {
    // 这些都应该失败，因为无法访问基类的private using namespace A
    // expected-error@+1 {{use of undeclared identifier 'value'}}
    int x = value;
    // expected-error@+1 {{use of undeclared identifier 'func'}}
    func();
    // expected-error@+1 {{unknown type name 'Helper'}}
    Helper h;
  }
};

// 测试多重继承情况
class Base1 {
private:
  using namespace A;
public:
  void useA() {
    int x = value;  // OK: A::value
  }
};

class Base2 {
private:
  using namespace B;
public:
  void useB() {
    int x = value;  // OK: B::value
  }
};

class MultiDerived : public Base1, public Base2 {
public:
  void test() {
    // 这些都应该失败，因为无法访问任何基类的private using
    // expected-error@+1 {{use of undeclared identifier 'value'}}
    int x = value;
    // expected-error@+1 {{use of undeclared identifier 'func'}}
    func();
  }
};

// 测试嵌套类
class Outer {
private:
  using namespace A;
  
  class Inner {
  public:
    void test() {
      // expected-error@+1 {{use of undeclared identifier 'value'}}
      int x = value;  // 应该失败，内部类无法访问外部类的private using
    }
  };
  
public:
  void test() {
    int x = value;  // OK: 可以在同一个类中访问
  }
}; 