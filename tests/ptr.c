int add(int* a, int* b) {
    return *a + *b;
}

int main() {
    int x = 10;
    int y = 20;

    int* px = &x;
    int* py = &y;

    printf("x: %d\n", x);
    printf("y: %d\n", y);
    printf("x + y: %d\n", add(px, py));

    *px = 99;
    printf("x after *px = 99: %d\n", x);

    return 0;
}
