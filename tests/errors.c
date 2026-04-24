
//hello 
int main() {
/*
 *
 *  hiii *** main()
 */
    int x = 10;

    recurse(x);

    return 0;
}

void recurse(int n) {
    if(n == 0) return;

    printf("%d ", n);
    recurse(n-1);
}

