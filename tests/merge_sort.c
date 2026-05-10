void merge(int* arr, int left, int mid, int right) {
    int i = left;
    int j = mid + 1;
    int temp[64];
    int k = 0;

    while (i <= mid && j <= right) {
        if (arr[i] <= arr[j]) {
            temp[k] = arr[i];
            i++;
            k++;
        } else {
            temp[k] = arr[j];
            j++;
            k++;
        }
    }

    while (i <= mid) {
        temp[k] = arr[i];
        i++;
        k++;
    }

    while (j <= right) {
        temp[k] = arr[j];
        j++;
        k++;
    }

    int m;
    for (m = 0; m < k; m++) {
        arr[left + m] = temp[m];
    }
}

void merge_sort(int* arr, int left, int right) {
    if (left >= right) {
        return;
    }
    int mid = left + (right - left) / 2;
    merge_sort(arr, left, mid);
    merge_sort(arr, mid + 1, right);
    merge(arr, left, mid, right);
}

void print_array(int* arr, int n) {
    int i;
    for (i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main() {
    int arr[10];
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

    printf("unsorted: ");
    print_array(arr, 10);

    merge_sort(arr, 0, 9);

    printf("sorted:   ");
    print_array(arr, 10);

    return 0;
}
