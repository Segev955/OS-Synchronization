#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "Process.h"

typedef struct Stack {
    char stack[1024];
    struct Stack *next;
} Stack, *pStack;

int count = 0;

void push(char *str, pStack *head) {
    if (count == 1024) {
        printf("ERROR: Stack full\n");
        return;
    }
    pStack node = (pStack) (malloc(sizeof(Stack)));
    if (node == NULL) {
        perror("Malloc failed");
        exit(0);
    }
    bzero(node->stack, 1024);
    strcpy(node->stack, str); //input data
    node->next = *head;
    *head = node;
    printf("'%s' pushed to stack\n", str);
    count++;
}

void pop(pStack *head) {
    if (count == 0) {
        printf("ERROR: Stack empty\n");
        return;
    }
    pStack tmp = *head;
    *head = (*head)->next;
    printf("'%s' poped\n", tmp->stack);
    free(tmp);
    count--;
}

void top(pStack *head) {
    if (count == 0) {
        printf("ERROR: Stack empty\n");
        return;
    }
    printf("OUTPUT: ");
    printf("%s\n", (*head)->stack);
}
int checkSUB(char e[], char s[]) {
    if (strlen(s) < strlen(e))
        return 0;
    for (int i = 0; i < strlen(e); ++i) {
        if (s[i] != e[i])
            return 0;
    }
    return 1;
}

int main() {
    pStack head = NULL;
    char text[1024];
    while (1) {
        char str[1024];
        bzero(str,1024);
        bzero(text,1024);
        for (int i = 0; i < 1024; i++) {
            if (text[i - 1] == '\n') {
                text[i-1] = '\0';
                break;
            }
            scanf("%c", &text[i]);
        }
        if (checkSUB("PUSH ", text)) {
            for (int i = 5; i < strlen(text); ++i) {
                str[i-5] = text[i];
            }
            push(str,&head);
        }
        else if (checkSUB("POP", text)) {
            pop(&head);
        }
        else if (checkSUB("TOP", text)) {
            top(&head);
        }
        else if (checkSUB("STOP", text)) {
            printf("See Ya");
            return 1;
        }
    }
    return 0;
}
