int power(int base, int exp) {
    int result = 1;
    int i;
    for (i = 0; i < exp; i++) {
        result = result * base;
    }
    return result;
}

int main() {
    printf("%d\n", power(2, 10));
    printf("%d\n", power(3, 5));
    printf("%d\n", power(5, 3));
    return 0;
}
