#include <stdio.h>
#include <stdlib.h>

#define ROWS 2
#define COLS 3

static void show_2d_array(int arr[ROWS][COLS])
{
    printf("2D array contiguous: ");
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            printf("%d ", arr[r][c]);
    printf("\n  &arr[0][0]=%p &arr[1][0]=%p step=%td\n",
           (void *)&arr[0][0], (void *)&arr[1][0],
           (char *)&arr[1][0] - (char *)&arr[0][0]);
}

static void show_jagged(int **m, int rows, int cols)
{
    printf("int** jagged (rows separate): ");
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            printf("%d ", m[r][c]);
    printf("\n  row0=%p row1=%p (likely NOT adjacent)\n",
           (void *)m[0], (void *)m[1]);
}

int main(void)
{
    int arr[ROWS][COLS] = {{1, 2, 3}, {4, 5, 6}};
    show_2d_array(arr);

    int **m = malloc(ROWS * sizeof *m);
    for (int r = 0; r < ROWS; r++) {
        m[r] = malloc(COLS * sizeof **m);
        for (int c = 0; c < COLS; c++)
            m[r][c] = r * COLS + c + 1;
    }
    show_jagged(m, ROWS, COLS);

    for (int r = 0; r < ROWS; r++)
        free(m[r]);
    free(m);
    return 0;
}
