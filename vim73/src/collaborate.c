// TODO(zpotter): legal boilerplate

/*
 * Functions to asynchronously queue realtime edits and to notify vim's main
 * loop of mutations to process.
 *
 * Notice we don't use vim's alloc or vim_free here due to thread-safety.
 */

#include "vim.h"
#include <pthread.h>

#include "collab_structs.h"
#include "collab_util.h"

/* The global queue to hold edits for loaded file buffers. */
editqueue_T collab_queue = { 
  .head = NULL,
  .tail = NULL,
  .mutex = PTHREAD_MUTEX_INITIALIZER
};

/* Sequence of keys interpretted as a collaborative event */
static const char_u collab_keys[3] = { CSI, KS_EXTRA, KE_COLLABEDIT };
static const size_t collab_keys_length = 
  sizeof(collab_keys) / sizeof(collab_keys[0]);
/* The next key in the sequence to send to the user input buffer */
static int next_key_index = -1;

// TODO(zpotter):
// Provides methods for communicating with the JS layer
// Translates between PPB_Var's and collabedit_T's
// void send_collabedit(collabedit_T *ev);
// bool receive_collabedit(PPB_Var *var);

/*
 * Places a collabedit_T in a queue of pending edits. Takes ownership of cedit
 * and frees it after it has been applied to the buffer. This function is 
 * thread-safe and may block until the shared queue is safe to modify.
 */
void collab_enqueue(editqueue_T *queue, collabedit_T *cedit) {
  editnode_T *node = (editnode_T*)malloc(sizeof(editnode_T));
  node->edit = cedit;
  node->next = NULL;

  // Wait for exclusive access to the queue
  pthread_mutex_lock(&(queue->mutex));

  // Enqueue!
  if (queue->tail == NULL) {
    // Queue was empty
    queue->head = node;
  } else {
    // Add to end
    queue->tail->next = node;
  }
  queue->tail = node;

  // Release exclusive access to queue
  pthread_mutex_unlock(&(queue->mutex));
}

/*
 * Applies a single collabedit_T to the collab_buf. Frees cedit when done.
 */
static void applyedit(collabedit_T *cedit) {
  // First select the right collaborative buffer
  buf_T *oldbuf = curbuf;
  if (curbuf != cedit->file_buf) set_curbuf(cedit->file_buf, DOBUF_GOTO);

  // Apply edit depending on type
  switch (cedit->type) {
    case COLLAB_TEXT_DELETE:
      // Delete the specified line from the buffer
      ml_delete(cedit->edit.text_delete.line, 0);
      // Update cursor and mark for redraw
      deleted_lines_mark(cedit->edit.text_delete.line, 1);
      break;

    case COLLAB_TEXT_INSERT:
      // Add the new line to the buffer
      ml_append(cedit->edit.text_insert.line, cedit->edit.text_insert.text, 0, FALSE);
      // Update cursor and mark for redraw.
      // Just appended a line bellow (text_insert.line + 1)
      appended_lines_mark(cedit->edit.text_insert.line + 1, 1);
      // Free up union specifics
      free(cedit->edit.text_insert.text);
      break;
  }
  // Switch back to old buffer if necessary 
  if (curbuf != oldbuf) set_curbuf(oldbuf, DOBUF_GOTO);

  // Done with collabedit_T, so free it
  free(cedit);
}

/*
 * Applies all currently pending collabedit_T mutations to the vim file buffer
 * This function should only be called from vim's main thread when it is safe
 * to modify the file buffer.
 */
void collab_applyedits(editqueue_T *queue) {
  editnode_T *edits_todo = NULL; 
  editnode_T *lastedit = NULL;
  // Dequeue entire edit queue for processing
  // Wait for exclusive access to the queue
  pthread_mutex_lock(&(queue->mutex));  
  edits_todo = queue->head;
  // Clear queue
  queue->head = NULL;
  queue->tail = NULL;
  pthread_mutex_unlock(&(queue->mutex));

  // Apply all pending edits
  while (edits_todo) {
    // Process the collabedit_T
    applyedit(edits_todo->edit);
    lastedit = edits_todo;
    edits_todo = edits_todo->next;
    free(lastedit);
  }
}

/*
 * When called, if there are pending edits to process, this will copy up to
 * "maxlen" characters of a special sequence into "buf". When the sequence is
 * read by vim's user input processor, it will trigger a call to 
 * collab_process. Seems a little hacky, but this is how vim processes special
 * events. This function should only be called from vim's main thread.
 * Returns the number of characters copied into the buffer.
 */
int collab_inchar(char_u *buf, int maxlen, editqueue_T *queue) {
  // If not in the process of sending the complete sequence... 
  if (next_key_index < 0) {
    pthread_mutex_lock(&(queue->mutex));
    if (queue->head) {
      // There are pending edits, so the sequence should begin
      next_key_index = 0;
    }
    pthread_mutex_unlock(&(queue->mutex));
  }

  int nkeys = 0;
  // If copying the sequence...
  if (next_key_index >= 0) {
    // Determine number of keys to copy
    nkeys = collab_keys_length - next_key_index;
    if (nkeys > maxlen) {
      nkeys = maxlen;
    }

    memcpy(buf, (collab_keys+next_key_index), nkeys*sizeof(char_u));

    // Determine the next key to copy if not complete    
    next_key_index += nkeys;
    if (next_key_index >= collab_keys_length) {
      next_key_index = -1;
    }
  }

  // Return number of copied keys
  return nkeys;
}

// Declaration in collab_util.h
collabedit_T* collab_dequeue(editqueue_T *queue) {
  if (queue->head == NULL) return NULL;

  editnode_T* oldhead = queue->head;
  queue->head = oldhead->next;

  if (queue->head == NULL) queue->tail = NULL;

  collabedit_T *popped = oldhead->edit;
  free(oldhead);

  return popped;
}
