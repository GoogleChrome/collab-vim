// TODO(zpotter): Legal boilerplate

/*
 * Structs for representing collaborator information and file edit events.
 */

#ifndef VIM_COLLAB_STRUCTS_H_
#define VIM_COLLAB_STRUCTS_H_

#include <pthread.h>
#include "vim.h"

/*
 * Enumerations for different types of collaborative edits.
 */
typedef enum {
  COLLAB_APPEND_LINE, /* A new line was added to the document. */
  COLLAB_INSERT_TEXT, /* Text was inserted into an existing line. */
  COLLAB_REMOVE_LINE, /* A line was removed from the document. */
  COLLAB_DELETE_TEXT, /* Text was deleted from an existing line. */
  COLLAB_REPLACE_LINE /* A line was replaced with new text. */
} collabtype_T;

/*
 * Represents a number of basic file edits.
 * Line numbers in this struct are 1-based, meaning line 1 is the first line.
 */
typedef struct collabedit_S {
  collabtype_T type;  /* The type determines which union member to use. */
  buf_T *file_buf;    /* The collaborative file buffer this applies to. */
  union {
    struct {          /* Type: COLLAB_APPEND_LINE */
      size_t line;    /* The line to add after. Line 0 adds a new 1st line. */
      char_u *text;   /* The text to initialize the line with. */
    } append_line;
  
    struct {          /* Type: COLLAB_INSERT_TEXT */
      size_t line;    /* The line to insert text into. */
      size_t index;   /* The character in the line to insert before. */
      char_u *text;   /* The text to insert. */
    } insert_text;
    
    struct {          /* Type: COLLAB_REMOVE_LINE */
      size_t line;    /* The line to remove from the document. */
    } remove_line;

    struct {          /* Type: COLLAB_DELETE_TEXT */
      size_t line;    /* The line to remove text from. */
      size_t index;   /* The starting character in the line to remove. */
      size_t length;  /* The number of characters to remove. */
    } delete_text;

    struct {          /* Type: COLLAB_REPLACE_LINE */
      size_t line;    /* The line to replace. */
      char_u * text;  /* The new contents of the line. */
    } replace_line;
  };
} collabedit_T;

/*
 * A node in the queue.
 */
typedef struct editnode_S {
  collabedit_T *edit;       /* An edit in the queue. */
  struct editnode_S *next;  /* The next edit in the queue, or NULL. */
} editnode_T;

/*
 * The queue and its mutex.
 */
typedef struct editqueue_S {
  editnode_T *head;       /* The head of the queue. */
  editnode_T *tail;       /* The tail of the queue. */
  
  pthread_mutex_t mutex;  /* Lock this mutex before modifying queue. */

  int event_write_fd;     /* After an enqueue, this file descriptor is written
                              to, causing vim's main thread to end waiting for
                              user input. */ 
  int event_read_fd;      /* File descriptor that contains a byte (any value)
                              for each event in the queue. */
} editqueue_T;

#endif // VIM_COLLAB_STRUCTS_H_

