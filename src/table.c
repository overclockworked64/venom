#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "table.h"

static void list_insert(Bucket **head, char *key, double item) {
    /* create a new node */
    Bucket *new_node = malloc(sizeof(Bucket));
    new_node->key = key;
    new_node->value = item;
    new_node->next = NULL;

    /* handle the edge case when the list is empty */
    if (*head == NULL) {
        *head = new_node;
    } else {
        /* go to the end of the list */
        Bucket *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
    
        /* store the new node at the end */
        current->next = new_node; 
    }
}

static void list_free(Bucket *head) {
    Bucket *tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

static double list_find(Bucket *head, char *item) {
    while (head != NULL) {
        if (strcmp(head->key, item) == 0) return head->value;
        head = head->next;
    }
    return -1;
}

static uint32_t hash(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

void table_insert(Table *table, char *key, double value) {
    int index = hash(key, strlen(key)) % 1024;
    list_insert(&table->data[index], key, value);
}

double table_get(const Table *table, char *key) {
    int index = hash(key, strlen(key)) % 1024;
    if (strcmp(table->data[index]->key, key) == 0) {
        return list_find(table->data[index], key);
    }
    return -1;
}

void table_free(const Table *table) {
    for (int i = 0; i < 1024; i++) {
        if (table->data[i] != NULL) {
            list_free(table->data[i]);
        }
    }
}

int main() {
    Table table = {0};
    table_insert(&table, "spam", 1.3);
    double egg = table_get(&table, "spam");
    printf("%f", egg);
    table_free(&table);
}