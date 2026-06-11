#include <stdlib.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

int main() {
    Node *head = malloc(sizeof(Node));
    head->next = malloc(sizeof(Node));

    free(head);

    return 0;
}