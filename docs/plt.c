
typedef int (*bar)();

volatile bar _bar;

int foo() {
    return _bar();
}
