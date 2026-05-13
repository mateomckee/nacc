void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void print_array(int* arr, int n) {
    int i;
    for (i = 0; i < n; i++) {
        printf("%3d", arr[i]);
    }
    printf("\n");
}

void print_partition(int* arr, int n, int pivot_idx) {
    int i;
    for (i = 0; i < n; i++) {
        printf("%3d", arr[i]);
    }
    printf("\n");
    for (i = 0; i < n; i++) {
        if (i == pivot_idx) {
            printf("  ^");
        } else {
            printf("   ");
        }
    }
    printf(" <-- pivot\n");
}

int partition(int* arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j;

    for (j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

void quicksort_verbose(int* arr, int low, int high, int total) {
    if (low < high) {
        int pivot_idx = partition(arr, low, high);

        printf("[*] partition [%d..%d] pivot=%d\n", low, high, arr[pivot_idx]);
        print_partition(arr, total, pivot_idx);
        printf("\n");

        quicksort_verbose(arr, low, pivot_idx - 1, total);
        quicksort_verbose(arr, pivot_idx + 1, high, total);
    }
}

int main() {
    int arr[12];
    arr[0]  = 64;
    arr[1]  = 34;
    arr[2]  = 25;
    arr[3]  = 12;
    arr[4]  = 22;
    arr[5]  = 11;
    arr[6]  = 90;
    arr[7]  = 7;
    arr[8]  = 55;
    arr[9]  = 3;
    arr[10] = 48;
    arr[11] = 77;

    printf("================================\n");
    printf("  nacc compiler demo\n");
    printf("  quicksort on AArch64\n");
    printf("================================\n");
    printf("\n");
    printf("[*] array size:   12 elements\n");
    printf("[*] target arch:  AArch64\n");
    printf("[*] compiled by:  nacc\n");
    printf("\n");
    printf("[>] unsorted: ");
    print_array(arr, 12);
    printf("\n");
    printf("[*] sorting...\n\n");

    quicksort_verbose(arr, 0, 11, 12);

    printf("[>] sorted:   ");
    print_array(arr, 12);
    printf("\n");
    printf("[+] done.\n");
    printf("================================\n");

    return 0;
}

