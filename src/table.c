#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "table.h"
#include "util.h"

static void list_insert(Bucket **head, char *key, Object item) {
    /* Create a new node. */
    Bucket *new_node = malloc(sizeof(Bucket));
    new_node->key = key;
    new_node->obj = item;
    new_node->next = NULL;

    /* Handle the edge case when the list is empty. */
    if (*head == NULL) {
        *head = new_node;
    } else {
        /* Go to the end of the list. */
        Bucket *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
    
        /* Store the new node at the end. */
        current->next = new_node; 
    }
}

static void list_free(Bucket *head) {
    Bucket *tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp->key);
        free(tmp);
    }
}

static Object *list_find(Bucket *head, const char *item) {
    while (head != NULL) {
        if (strcmp(head->key, item) == 0) return &head->obj;
        head = head->next;
    }
    return NULL;
}

static uint32_t hash(const char *key, int length) {
    /* copy-paste from 'crafting interpreters' */
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

void table_insert(Table *table, const char *key, Object obj) {
    char *k = own_string(key);
    int index = hash(k, strlen(k)) % 1024;
    if (list_find(table->data[index], k) == NULL) {
        list_insert(&table->data[index], k, obj);
    } else {
        /* If the key is already in the list, change its value. */
        Bucket *head = table->data[index];
        while (head != NULL) {
            if (strcmp(head->key, k) == 0) {
                table->data[index]->obj = obj;
            }
            head = head->next;
        }
        free(k);
    }
}

Object *table_get(const Table *table, const char *key) {
    int index = hash(key, strlen(key)) % 1024;
    return list_find(table->data[index], key);
}

void table_free(const Table *table) {
    for (size_t i = 0; i < sizeof(table->data) / sizeof(table->data[0]); i++) {
        if (table->data[i] != NULL) {
            list_free(table->data[i]);
        }
    }
}
