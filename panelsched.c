/* Panelist Scheduler
 *
 * Excpects an input CSV with the first row to be arbitrary table information,
 * slot names in the second row except the first column, which can also be
 * arbitrary table information, and panelist names in the first column of all
 * subsequent rows. Panelist availability is denoted with an 'x' in the position
 * indexed by the panelist's row and the column(s) corresponding to the time
 * slot(s) the panelist is able to work. All other fields should be empty.
 *
 * The output is a CSV file that is written over the input file. Each panelist
 * will have no more than one 'x' in its row, and each slot will have no more
 * than MAX_PANELISTS (as defined below) 'x's in its column.
 *
 * Ex. input                           Ex. output
 *  ________________________________    ________________________________
 * | Blah  | Blah  | Blah  | Blah  |   | Blah  | Blah  | Blah  | Blah  |
 * |_______|_______|_______|_______|   |_______|_______|_______|_______|
 * | Blah  | Slot1 | Slot2 | Slot3 |   | Blah  | Slot1 | Slot2 | Slot3 |
 * |_______|_______|_______|_______|   |_______|_______|_______|_______|
 * | Name1 | x     | x     |       |   | Name1 | x     |       |       |
 * |_______|_______|_______|_______|   |_______|_______|_______|_______|
 * | Name2 | x     |       | x     |   | Name2 | x     |       |       |
 * |_______|_______|_______|_______|   |_______|_______|_______|_______|
 * | Name3 |       | x     |       |   | Name3 |       | x     |       |
 * |_______|_______|_______|_______|   |_______|_______|_______|_______|
 * | Name4 |       | x     | x     |   | Name4 |       | x     |       |
 * |_______|_______|_______|_______|   |_______|_______|_______|_______|
 * | Name5 | x     | x     | x     |   | Name5 |       |       | x     |
 * |_______|_______|_______|_______|   |_______|_______|_______|_______|
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PANELISTS  8 /* Maximum panelists per slot */
#define INIT_LIST_SIZE 4 /* Initial list sizes (as power of 2) */

/******************************************************************************/
/****************************  Vertex Definitions  ****************************/
/******************************************************************************/

struct vertex;
typedef struct vertex vertex;
struct vertex {
  char *name;
  int index;
  int num_adj;
  int adj_size; /* Adjacency list size (as power of 2) */
  vertex **adj; /* Adjacent vertex list */
  vertex *pair; /* Connected vertex */
  unsigned int dist;
};

typedef struct {
  int size;
  int list_size; /* Vertex list size (as power of 2) */
  vertex **list; /* Vertex list */
} vertex_array;

/* Creates a new vertex */
vertex * new_vertex(char *name, int name_len) {
  vertex *v;

  v = (vertex *)malloc(sizeof(vertex));
  v->name = (char *)malloc((name_len + 1) * sizeof(char));
  strncpy(v->name, name, name_len);
  v->name[name_len] = '\0';
  v->index = -1;
  v->num_adj = 0;
  v->adj_size = INIT_LIST_SIZE;
  v->adj = (vertex **)malloc(sizeof(vertex *) << INIT_LIST_SIZE);
  v->pair = NULL;
  v->dist = 0;

  return v;
}

/* Creates a new vertex array */
vertex_array * new_vertex_array() {
  vertex_array *va;

  va = (vertex_array *)malloc(sizeof(vertex_array));
  va->size = 0;
  va->list_size = INIT_LIST_SIZE;
  va->list = (vertex **)malloc(sizeof(vertex *) << INIT_LIST_SIZE);

  return va;
}

/* Deletes a vertex */
void del_vertex(vertex *v) {
  free(v->name);
  free(v->adj);
  free(v);
}

/* Deletes a vertex array */
void del_vertex_array(vertex_array *va) {
  free(va->list);
  free(va);
}

/* Doubles vertex's adjacency list capacity */
void resize_adj(vertex *v) {
  int old_size, new_size, i;
  vertex **old_adj, **new_adj;

  old_size = v->adj_size;
  new_size = old_size + 1;
  old_adj = v->adj;
  new_adj = (vertex **)malloc(sizeof(vertex *) << new_size);

  /* Copy the old adjacency list and free it */
  for (i = 0; i < 1 << old_size; i++)
    new_adj[i] = old_adj[i];
  free(old_adj);

  v->adj_size = new_size;
  v->adj = new_adj;
}

/* Doubles verex array's list capacity */
void resize_vertex_array(vertex_array *va) {
  int old_size, new_size, i;
  vertex **old_list, **new_list;

  old_size = va->list_size;
  new_size = old_size + 1;
  old_list = va->list;
  new_list = (vertex **)malloc(sizeof(vertex *) << new_size);

  /* Copy the old vertex list and free it */
  for (i = 0; i < 1 << old_size; i++)
    new_list[i] = old_list[i];
  free(old_list);

  va->list_size = new_size;
  va->list = new_list;
}

/* Adds vertex u to vertex v's adjacency list and vice-versa */
void add_adj(vertex *v, vertex *u) {
  /* Resize the adjacency list if necessary */
  if (v->num_adj == 1 << v->adj_size)
    resize_adj(v);
  if (u->num_adj == 1 << u->adj_size)
    resize_adj(u);

  v->adj[v->num_adj] = u;
  u->adj[u->num_adj] = v;
  v->num_adj++;
  u->num_adj++;
}

/* Adds vertex v to vertex array va */
void add_vertex(vertex_array *va, vertex *v) {
  /* Resize the vertex array if necessary */
  if (va->size == 1 << va->list_size)
    resize_vertex_array(va);

  va->list[va->size] = v;
  va->size++;
}

/******************************************************************************/
/****************************  Queue Definitions  *****************************/
/******************************************************************************/

typedef struct {
  int size;
  int list_size; /* Queue list size (as power of 2) */
  vertex **list; /* Queue list */
  int head;
  int tail;
} queue;

/* Creates a new queue */
queue * new_queue() {
  queue *q;

  q = (queue *)malloc(sizeof(queue));
  q->size = 0;
  q->list_size = INIT_LIST_SIZE;
  q->list = (vertex **)malloc(sizeof(vertex *) << INIT_LIST_SIZE);
  q->head = 0;
  q->tail = 0;

  return q;
}

/* Deletes a queue */
void del_queue(queue *q) {
  free(q->list);
  free(q);
}

/* Doubles queue's capacity */
void resize_queue(queue *q) {
  int size, old_list_size, new_list_size, old_head, i;
  vertex **old_list, **new_list;

  size = q->size;
  old_list_size = q->list_size;
  new_list_size = old_list_size + 1;
  old_head = q->head;
  old_list = q->list;
  new_list = (vertex **)malloc(sizeof(vertex *) << new_list_size);

  /* Copy the old queue list (accounting for wraparound) and free it */
  for (i = 0; i < size; i++)
    new_list[i] = old_list[(i + old_head) & ~(1 << old_list_size)];
  free(old_list);

  q->list_size = new_list_size;
  q->list = new_list;
  q->head = 0;
  q->tail = size;
}

/* Enqueues a vertex */
void enqueue(queue *q, vertex *v) {
  /* Resize the queue list if necessary */
  if (q->size == (1 << q->list_size) - 1)
    resize_queue(q);

  q->list[q->tail] = v;
  q->tail = (q->tail + 1) & ~(1 << q->list_size); /* Account for wraparound */
  q->size++;
}

/* Dequeues a vertex */
vertex * dequeue(queue *q) {
  vertex *v;

  if (!q->size)
    return NULL;

  v = q->list[q->head];
  q->head = (q->head + 1) & ~(1 << q->list_size); /* Account for wraparound */
  q->size--;

  return v;
}

/******************************************************************************/
/************************  String Buffer Definitions  *************************/
/******************************************************************************/

typedef struct {
  int size;
  int list_size; /* Char list size (as power of 2) */
  char *list;    /* Char list */
} buffer;

/* Creates a new buffer */
buffer * new_buffer() {
  buffer *buf;

  buf = (buffer *)malloc(sizeof(buffer));
  buf->size = 0;
  buf->list_size = INIT_LIST_SIZE;
  buf->list = (char *)malloc(sizeof(char) << INIT_LIST_SIZE);

  return buf;
}

/* Deletes a buffer */
void del_buffer(buffer *buf) {
  free(buf->list);
  free(buf);
}

/* Resets a buffer */
inline void reset_buffer(buffer *buf) {
  buf->size = 0;
}

/* Doubles buffer's list capacity */
void resize_buffer(buffer *buf) {
  int old_size, new_size, i;
  char *old_list, *new_list;

  old_size = buf->list_size;
  new_size = old_size + 1;
  old_list = buf->list;
  new_list = (char *)malloc(sizeof(char) << new_size);

  /* Copy the old char list and free it */
  for (i = 0; i < 1 << old_size; i++)
    new_list[i] = old_list[i];
  free(old_list);

  buf->list_size = new_size;
  buf->list = new_list;
}

/* Adds char c to buffer buf */
void add_char(buffer *buf, char c) {
  /* Resize the buffer if necessary */
  if (buf->size == 1 << buf->list_size)
    resize_buffer(buf);

  buf->list[buf->size] = c;
  buf->size++;
}

/******************************************************************************/
/*************************  Hopcroft-Karp Functions  **************************/
/*****  http://en.wikipedia.org/wiki/Hopcroft-Karp_algorithm#Pseudocode  ******/
/******************************************************************************/

int hk_bfs(queue *q, vertex_array *panelists, vertex *nil) {
  vertex *p, *s;
  int i;

  for (i = 0; i < panelists->size; i++) {
    p = panelists->list[i];
    if (p->pair == nil) {
      p->dist = 1;
      enqueue(q, p);
    } else
      p->dist = 0;
  }

  nil->dist = 0;
  while (q->size) {
    p = dequeue(q);
    for (i = 0; i < p->num_adj; i++) {
      s = p->adj[i];
      if (s->pair->dist == 0) {
        s->pair->dist = p->dist ? p->dist + 1 : 0;
        enqueue(q, s->pair);
      }
    }
  }

  return nil->dist;
}

int hk_dfs(vertex *p, vertex *nil) {
  vertex *s;
  int i;

  if (p == nil)
    return 1;

  for (i = 0; i < p->num_adj; i++) {
    s = p->adj[i];
    if (s->pair->dist == (p->dist ? p->dist + 1 : 0)) {
      if (hk_dfs(s->pair, nil)) {
        s->pair = p;
        p->pair = s;
        return 1;
      }
    }
  }

  p->dist = 0;
  return 0;
}

int hk(queue *q, vertex_array *panelists, vertex_array *slots, vertex *nil) {
  vertex *p;
  int i, matches = 0;

  for (i = 0; i < panelists->size; i++)
    panelists->list[i]->pair = nil;
  for (i = 0; i < slots->size; i++)
    slots->list[i]->pair = nil;

  while (hk_bfs(q, panelists, nil)) {
    for (i = 0; i < panelists->size; i++) {
      p = panelists->list[i];
      if (p->pair == nil && hk_dfs(p, nil))
        matches++;
    }
  }

  return matches;
}

/******************************************************************************/
/***********************************  Main  ***********************************/
/******************************************************************************/

int main(int argc, char **argv) {
  vertex_array *headers, *panelists, *slots;
  vertex *nil;
  queue *q;
  FILE *csv;
  int i, j;
  int c;
  int row, col;
  int matches;
  buffer *buf;
  vertex *v;

  if (argc != 2) {
    printf("USAGE: %s [CSV FILE]\n", argv[0]);
    return 0;
  }

  /* Try to open the CSV file */
  csv = fopen(argv[1], "rb");
  if (csv == NULL) {
    printf("Unable to open file for reading: %s\n", argv[1]);
    exit(1);
  }

  /* Create the global data structures */
  q = new_queue();
  headers = new_vertex_array();
  panelists = new_vertex_array();
  slots = new_vertex_array();
  nil = new_vertex("", 0);

  /* Create the string buffer */
  buf = new_buffer();

  /* Parse the file */
  row = 0;
  col = 0;
  do {
    c = fgetc(csv);
    if (c == '\r') continue;
    if (c == ',' || c == '\n') {
      add_char(buf, '\0');
      if (row == 0 || (c == ',' && row == 1 && col == 0)) { /* Header */
        v = new_vertex(buf->list, buf->size);
        add_vertex(headers, v);
      } else if (row == 1)                                  /* Slot */
        for (i = 0; i < MAX_PANELISTS; i++) {
          v = new_vertex(buf->list, buf->size);
          v->index = col - 1;
          add_vertex(slots, v);
        }
      else if (col == 0) {                                  /* Panelist */
        v = new_vertex(buf->list, buf->size);
        add_vertex(panelists, v);
      } else if (buf->list[0] == 'x')                       /* Availabile */
        for (i = 0; i < MAX_PANELISTS; i++)
          add_adj(panelists->list[row - 2],
            slots->list[MAX_PANELISTS * (col - 1) + i]);
      reset_buffer(buf);
      if (c == ',')
        col++;
      else {
        col = 0;
        row++;
      }
    } else
      add_char(buf, c);
  } while (c != EOF);

  /* Close the file */
  fclose(csv);

  /* Run Hopcroft-Karp */
  matches = hk(q, panelists, slots, nil);

  /* Re-open the file for writing */
  csv = fopen(argv[1], "wb");
  if (csv == NULL) {
    printf("Unable to open file for writing: %s\n", argv[1]);
    exit(1);
  }

  /* Write the output */
  fprintf(csv, "%s", headers->list[0]->name);
  for (i = 1; i < headers->size - 1; i++)
    fprintf(csv, ",%s", headers->list[i]->name);
  fprintf(csv, "\n%s", headers->list[headers->size - 1]->name);
  for (i = 0; i < slots->size; i += MAX_PANELISTS)
    fprintf(csv, ",%s", slots->list[i]->name);
  fprintf(csv, "\n");
  for (i = 0; i < panelists->size; i++) {
    fprintf(csv, "%s", panelists->list[i]->name);
    for (j = 0; j < slots->size / MAX_PANELISTS; j++) {
      fprintf(csv, ",");
      if (panelists->list[i]->pair->index == j)
        fprintf(csv, "x");
    }
    fprintf(csv, "\n");
  }

  /* Close the file */
  fclose(csv);

  /* Delete data structures */
  for (i = 0; i < slots->size; i++)
    del_vertex(slots->list[i]);
  for (i = 0; i < panelists->size; i++)
    del_vertex(panelists->list[i]);
  for (i = 0; i < headers->size; i++)
    del_vertex(headers->list[i]);
  del_buffer(buf);
  del_vertex(nil);
  del_vertex_array(slots);
  del_vertex_array(panelists);
  del_vertex_array(headers);
  del_queue(q);

  return 0;
}
