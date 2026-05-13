int N = 8;
int cols[8];
int count = 0;

int abs_diff(int a, int b) {
    int d = a - b;
    if (d < 0) {
        d = -d;
    }
    return d;
}

int safe(int row, int col) {
    int i = 0;
    while (i < row) {
        if (cols[i] == col) {
            return 0;
        }
        if (abs_diff(cols[i], col) == abs_diff(i, row)) {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

void print_board() {
    count = count + 1;
    printf("Solution %d:\n", count);

    int r = 0;
    while (r < N) {
        int c = 0;
        while (c < N) {
            if (cols[r] == c) {
                printf("Q ");
            }
            else {
                printf(". ");
            }
            c = c + 1;
        }
        printf("\n");
        r = r + 1;
    }
    printf("\n");
}

void solve(int row) {
    if (row == N) {
        print_board();
        return;
    }

    int c = 0;
    while (c < N) {
        if (safe(row, c)) {
            cols[row] = c;
            solve(row + 1);
        }
        c = c + 1;
    }
}

int main() {
    solve(0);
    printf("Total solutions for %d-queens: %d\n", N, count);
    return 0;
}
