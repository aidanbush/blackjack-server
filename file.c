/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Nov. 5, 17
 * File: file.c
 * Description: persistent file functions
 */

/* standard libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* system libraries */

/* project includes */
#include "game.h"
#include "file.h"

extern userlist_s userlist;

/* reads from a file if possible and adds valid users to the userlist */
int read_userlist_file(char *filename) {
    if (filename == NULL)
        return -1;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return -1;
    }

    char *line = NULL, *name, *money;
    uint32_t i_money;
    ssize_t read;
    size_t len = 0;

    //read from file
    while ((read = getline(&line, &len, file)) != -1) {
        name = strtok(line, ",");
        money = strtok(NULL, "");
        if (name == NULL || money == NULL)
            continue;
        i_money = atol(money);
        if (i_money == 0)
            continue;
        if (strlen(name) > PLAYER_NAME_LEN)
            continue;
        //check if valid name
        if (!valid_nick(name))
            continue;
        add_player_to_list(name, i_money);
    }

    fclose(file);

    if (line != NULL)
        free(line);
    return 1;
}

/* writes to the userlist to the given filename */
int write_userlist_file(char *filename) {
    if (filename == NULL)
        return -1;

    FILE *file;
    // overwrite existing file
    file = fopen(filename, "w");
    if (file == NULL)
        return -1;

    //of all users
    for (int i = 0; i < userlist.max_users; i++) {
        //if not null
        if (userlist.users[i] != NULL)
            fprintf(file, "%s,%d\n", userlist.users[i]->name, userlist.users[i]->money);
    }

    fclose(file);
    return 1;
}
