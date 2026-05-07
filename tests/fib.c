int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int i = 0;
    int result = 0;

    while (i < 10) {
        result = fib(i);
        printf("%d\n", result);
        i++;
    }

    return 0;
}
