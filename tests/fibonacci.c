char get_letter() {
    return 'g';
}

int global = 10*100+3;

int fib(int x) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int i = 0;
    while (i < 10) {
        printf("%d\n", fib(i));
        i++;
    }
    return 0;
}
