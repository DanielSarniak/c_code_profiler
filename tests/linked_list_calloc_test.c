#include <stdlib.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

int main() {
    Node *head = calloc(1, sizeof(Node));
    head->next = calloc(1, sizeof(Node));

    free(head);

    return 0;
}