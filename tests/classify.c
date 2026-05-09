int classify(int n) {
    if (n > 0) {
        return 1;
    }
    else if (n < 0) {
        return -1;
    }
    else {
        return 0;
    }
}

int main() {
    printf("%d\n", classify(5));
    printf("%d\n", classify(-3));
    printf("%d\n", classify(0));
    return 0;
}
