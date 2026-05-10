int main() {
    int arr[10];
    int n = 10;

    arr[0] = 64;
    arr[1] = 34;
    arr[2] = 25;
    arr[3] = 12;
    arr[4] = 22;
    arr[5] = 11;
    arr[6] = 90;
    arr[7] = 7;
    arr[8] = 55;
    arr[9] = 3;

    for(int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    printf("%p %p\n",&arr[0], &arr[1]);

    return 0;
}

