int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int i = 1;
    while (i <= 10) {
        printf("%d! = %d\n", i, factorial(i));
        i++;
    }
    return 0;
}
