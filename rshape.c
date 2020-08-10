/*  Main code for algorithm */

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "utils.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EXPORT_PREFIX EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT_PREFIX
#endif

#define TRUE 1
#define FALSE 0

#define EMPTY_VAL INT_MAX/2
#define MAX_PATHS 1 //prevent exponential ballon up of min paths in long sequences 
// TODO: find all paths with removing duplicates

typedef enum movement {
  RIGHT = 0x01u << 0,
  LEFT =  0x01u << 1,
} movement;

/* Model scores for each pair */
typedef struct {
  int mweight[4][4]; //weight of base pair match
  int gweight; //weight of gap
  int min_gap; //minimum number of base pairs in loop
  int scale; //for rescale, as use int to make comparisons easier vs floats
} model;

/* Stores info related to a seq structure problem */
typedef struct {
  char * seq;
  int seq_length;
  int * matrix; //matrix for storing pre-computed scores
} problem;

model current_model;
int min_score;
int num_paths;
char * paths[MAX_PATHS];

/* Map char to base pair int equivlanet */
int map_letter(char c) {
  switch(c) {
  case 'A':
    return 0;
  case 'C':
    return 1;
  case 'G':
    return 2;
  case 'U':
    return 3;
  }
  return -1;
}

/* Flips the index to bottom matrix index, as only use one matrix */
int flip_index(int index, problem * p) {
  return p->seq_length*p->seq_length - 1 - index;
}

int getmin(int a, int b) {
  if(a <= b) {
    return a;
  } else {
    return b;
  }
}

/* Comparison function for finding item in adtLinkedArray*/
int compare_str(void * a, void * b) {
  if(strcmp((char *) a,(char *) b) == 0) return 1;
  return 0;
}

/* Calculates score of two base pairs */
int calc_score(int i, int j, problem * p, model * m) {
  int score = m->mweight[map_letter(p->seq[i])][map_letter(p->seq[j])];
  if(abs(i - j) <= m->min_gap ||  score == 0) {
    return INT_MAX/2; //imposible cases set to MAX
  }
  return score;
}

//if inside a loop, scoring changes
int in_loop_helper(int i, int j, problem * p, model * m) {
  if(i == j) {
    return m->gweight*1;
  } else if(i-1 == j) {
    return m->gweight*2;
  } else if(p->matrix[flip_index(i*p->seq_length + j,p)] != EMPTY_VAL) {
    return p->matrix[flip_index(i*p->seq_length + j,p)];
  }
  //calculate new entry to get score
  int min = INT_MAX;
  min = getmin(min,in_loop_helper(i+1,j,p,m) + m->gweight);
  min = getmin(min,in_loop_helper(i,j-1,p,m) + m->gweight);
  min = getmin(min,in_loop_helper(i+1,j-1,p,m) + calc_score(i,j,p,m));
  for(int k = i+1; k < j; k++) {
    min = getmin(min,in_loop_helper(i,k,p,m)
                 + in_loop_helper(k+1,j,p,m));
  }
  p->matrix[flip_index(i*p->seq_length + j,p)] = min;
  return min;
}

int out_loop_helper(int i, int j, problem * p, model * m) {
  if(i == j || i-1 == j) {
    return 0;
  } else if(p->matrix[i*p->seq_length + j] != EMPTY_VAL) {
    return p->matrix[i*p->seq_length + j];
  }
  //calculate new entry to get score
  int min = INT_MAX;
  min = getmin(min, out_loop_helper(i+1,j,p,m));
  min = getmin(min,out_loop_helper(i,j-1,p,m));
  min = getmin(min,in_loop_helper(i+1,j-1,p,m) + calc_score(i,j,p,m));
  for(int k = i+1; k < j; k++) {
    min = getmin(min,out_loop_helper(i,k,p,m)
                 + out_loop_helper(k+1,j,p,m));
  }
  p->matrix[i*p->seq_length + j] = min;
  return min;
}

int loop_helper(int start_index, int end_index, problem * prb, model * mdl, int in_loop) {
  if(in_loop) return in_loop_helper(start_index, end_index, prb, mdl);
  return out_loop_helper(start_index, end_index, prb, mdl);
}

int calc_matrix(problem * p, model * m) {
  //find the min score.. which populates matrix for shape
  for(int j = 1; j < p->seq_length; j++) {
    for(int k = 0; k < p->seq_length-j; k++) {  //populate tables
      in_loop_helper(k,j+k,p, m);
      out_loop_helper(k,j+k,p, m);
    }
  }
  return out_loop_helper(0, p->seq_length-1,p, m);
}


int create_new_path(int start_index, int end_index, int path_index, problem *prb) {
  int new_path_index = num_paths;
  num_paths++;
  paths[new_path_index] = xmalloc(prb->seq_length+1);
  //copy old contents
  for(int k=0; k < start_index; k++) paths[new_path_index][k] = paths[path_index][k];
  for(int k=end_index+1; k < prb->seq_length; k++) paths[new_path_index][k] = paths[path_index][k];
  paths[new_path_index][prb->seq_length] = 0;
  return new_path_index;
}

//TODO: implement went right to remove duplicate paths
/* Path Finding Recurance */
void path_finder(int start_index, int end_index, int path_index, problem * prb, model * mdl, int in_loop, movement mv) {
  if(end_index - start_index <= 1 ) {
    paths[path_index][start_index] = '.';
    paths[path_index][end_index] = '.';
    return;
  }
  int gap_weight = in_loop ? mdl->gweight : 0;
  int first_match = TRUE;  
  // right shift
  if(loop_helper(start_index, end_index, prb, mdl, in_loop) == loop_helper(start_index+1, end_index, prb , mdl, in_loop) + gap_weight && mv & RIGHT) {
    paths[path_index][start_index] = '.'; //make path only go right after first right, to remove duplicate paths
    path_finder(start_index+1, end_index, path_index, prb, mdl, in_loop, RIGHT);
    first_match = FALSE;
  }
  // left shift
  if(loop_helper(start_index, end_index, prb, mdl, in_loop) == loop_helper(start_index, end_index-1, prb, mdl, in_loop) + gap_weight && mv & LEFT) {
    if(!first_match) {
      if(num_paths == MAX_PATHS) return;
      path_index = create_new_path(start_index, end_index, path_index, prb);
    }
    paths[path_index][end_index] = '.';
    path_finder(start_index, end_index-1, path_index, prb, mdl, in_loop, mv);
    first_match = FALSE;
  }
  // matching pair
  if(loop_helper(start_index, end_index, prb, mdl, in_loop) == loop_helper(start_index+1 , end_index-1, prb ,mdl, TRUE) + calc_score(start_index, end_index, prb, mdl)) {
    if(!first_match) {
      if(num_paths == MAX_PATHS) return;
      path_index = create_new_path(start_index, end_index, path_index, prb);
    }
    paths[path_index][start_index] = '(';
    paths[path_index][end_index] = ')';
    path_finder(start_index+1, end_index-1, path_index, prb, mdl, TRUE, RIGHT | LEFT);
    first_match = FALSE;
  }
  // split segments
  for(int split_index = start_index+1; split_index < end_index; split_index++) {
    if(loop_helper(start_index, end_index, prb, mdl, in_loop) == loop_helper(start_index, split_index, prb, mdl, in_loop) + loop_helper(split_index+1, end_index, prb, mdl, in_loop)) {
      if(!first_match) {
        if(num_paths == MAX_PATHS) return;
        path_index = create_new_path(start_index, end_index, path_index, prb);
      }
      first_match = FALSE;
      // count the extra paths so the pairs can be expanded
      int before_paths = num_paths;
      path_finder(start_index, split_index, path_index, prb, mdl, in_loop, LEFT);
      int left_paths = num_paths - before_paths; 
      path_finder(split_index+1, end_index, path_index, prb, mdl, in_loop, RIGHT);
      int right_paths = num_paths - left_paths - before_paths;
      //expand out the pairs now
      for(int j=0; j < left_paths && num_paths; j++) {
        //first pair is from original value
        for(int k=split_index+1; k <= end_index; k++) paths[before_paths+j][k] = paths[path_index][k];
        for(int k=0; k < right_paths && num_paths < MAX_PATHS; k++) {
            //create new path for pair, and copy over sub-sections
            int new_path_index = create_new_path(start_index, end_index, path_index, prb);
            for(int l=start_index; l <= split_index; l++) paths[new_path_index][l] = paths[before_paths+j][l];
            for(int l=split_index+1; l <= end_index; l++) paths[new_path_index][l] = paths[before_paths+left_paths+k][l];
        }
      }
    }
  }
}

void find_structures(problem * prb, model * mdl) {
  paths[0] = xmalloc(prb->seq_length+1);
  paths[0][prb->seq_length] = 0;
  num_paths = 1;
  path_finder(0, prb->seq_length-1, 0, prb, mdl, FALSE, LEFT | RIGHT);
  return;
}

/* Returns a min path */
char * EXPORT_PREFIX get_minimum(int index) {
  if(num_paths < index || index < 0) return NULL;
  return paths[index];
}

int EXPORT_PREFIX number_minimums() {
  return num_paths;
}

int EXPORT_PREFIX minimum_score() {
  return min_score;
}

/* Calculates all min structures for sequence */
int EXPORT_PREFIX find_minimums(char *seq) {

  regex_t seq_expr = {};
  const char * seq_pattern = "^[[:space:]]{0,}([AUCG]{1,})";
  if(regcomp(&seq_expr, seq_pattern, REG_EXTENDED) != 0) {
    printf("Aborting all, compiling regex failed\n");
    abort();
  }

  int seq_length = strlen(seq);

#ifdef DEBUG
  printf("Seq is (length=%d): %s\n",seq_length,seq);
#endif

  //clear old path storage
  for(int i=0; i < MAX_PATHS && paths[i]; i++) {
    xfree(paths[i]);
    paths[i] = NULL;
  }
  num_paths = 0;

  //matrix for storing precomputed values for speedup of recurances
  int * matrix = (int *) xmalloc(sizeof(int)*seq_length*seq_length);
  for(int i=0; i < seq_length*seq_length; i++) {
    matrix[i] = EMPTY_VAL;
  }

  problem current_problem;
  current_problem.seq = seq;
  current_problem.seq_length = strlen(seq);
  current_problem.matrix = matrix;

  min_score = calc_matrix(&current_problem,&current_model);
  find_structures(&current_problem,&current_model);

#ifdef DEBUG
  printf("The number of paths found is %d\n",num_paths);
#endif

  printf("Min is %d\n", min_score);
  printf("%d\n", num_paths);

  xfree(matrix);
  regfree(&seq_expr);
  return 0;
};

// assumes scaling of 10e-1
void EXPORT_PREFIX set_weights(int gap_penalty, int min_gap, int au_pair, int gu_pair, int gc_pair) {
  current_model.gweight = gap_penalty;
  current_model.min_gap = min_gap;
  current_model.mweight[map_letter('A')][map_letter('U')] = -au_pair;
  current_model.mweight[map_letter('U')][map_letter('A')] = -au_pair;
  current_model.mweight[map_letter('G')][map_letter('U')] = -gu_pair;
  current_model.mweight[map_letter('U')][map_letter('G')] = -gu_pair;
  current_model.mweight[map_letter('G')][map_letter('C')] = -gc_pair;
  current_model.mweight[map_letter('C')][map_letter('G')] = -gc_pair;
}

int main() {
  /* Set the defaults */
  set_weights(1,3,20,20,30);
  current_model.scale = -1;
  return 0;
}

/* Example Problem With Defaults
 *
rshape.js:2034 2
rshape.js:2034 ..(...)..
rshape.js:2034 (...)....
 * */
