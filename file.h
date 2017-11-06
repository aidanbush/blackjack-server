/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Nov. 5, 17
 * File: file.h
 * Description: persistent file functions
 */

#ifndef FILE_H
#define FILE_H

/* reads from a file if possible and adds valid users to the userlist */
int read_userlist_file(char *filename);

/* writes to the userlist to the given filename */
int write_userlist_file(char *filename);

#endif /* FILE_H */
