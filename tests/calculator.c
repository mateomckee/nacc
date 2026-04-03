int add(int a, int b) {
    return a+b;
}

int exponent(int a, int b) {
    int t = a;
    int i;
    for(i = 0; i < b; i++) {
        t *= a;
    }
    return t;
}

int main() {
    int num1 = add(10, 2);
    int num2 = 2;

    printf("%d\n", exponent(num2, num1));
    return 0;
}
