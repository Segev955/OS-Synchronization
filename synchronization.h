typedef struct Stack {
    char stack[1024];
    struct Stack *next;
} Stack, *pStack;
