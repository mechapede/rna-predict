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
#define EMPTY_VAL INT_MAX
#define PENALTY INT_MAX/4
#define MAX_PATHS 1000 //prevent exponential ballon up

#define MIN(X, Y)                \
({ typeof (X) x_ = (X);          \
   typeof (Y) y_ = (Y);          \
   (x_ < y_) ? x_ : y_; })

/* Movement States augmented by in_loop bool  */
typedef enum movstate {
  FREE = 0,
  RIGHT,  //only to right or loop, penalty if no loop, cannot split segments
  LEFT,   //only to left or loop, penalty if no loop, can split segments
  MOVE_MAX
} movstate;

/* Model scores for each pair */
typedef struct {
  int mweight[4][4];  //weight of base pair match A,U,G,C
  int gweight;        //weight of gap
  int min_gap;        //minimum number of base pairs in loop
  int scale;          //for rescale, as use ints to avoid precision issues
} model;

/* Stores info related to a seq structure problem */
typedef struct {
  char * seq;
  int seq_length;
  int * matrices[MOVE_MAX]; //matrices for storing pre-computed scores
} problem;

int map_letter(char c);
int flip_index(int index, problem * p);
int calc_score(int i, int j, problem * p, model * m);
int recurrence_calculator(int start_index, int end_index, movstate mov, int in_loop, problem * prb, model * mdl);
int create_new_path(int start_index, int end_index, int path_index, problem *prb);
void path_finder(int start_index, int end_index, movstate mov, int in_loop, int path_index, problem * prb, model * mdl);
int calc_matrix(problem * prb, model * mdl);
void find_structures(problem * prb, model * mdl);
char * EXPORT_PREFIX get_minimum(int index);
int EXPORT_PREFIX number_minimums();
int EXPORT_PREFIX minimum_score();
int EXPORT_PREFIX find_minimums(char *seq);


/* Globals */
regex_t seq_expr = {};
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

/* Calculates score of two base pairs */
int calc_score(int i, int j, problem * p, model * m) {
  int score = m->mweight[map_letter(p->seq[i])][map_letter(p->seq[j])];
  if(abs(i - j) <= m->min_gap ||  score == 0) {
    return PENALTY; //imposible cases set to high number
  }
  return score;
}

int recurrence_calculator(int start_index, int end_index, movstate mov, int in_loop, problem * prb, model * mdl) {
  int gap_weight = in_loop ? mdl->gweight : 0;
  int gap_size = (end_index - start_index)+1; //end index inclusive
  if(gap_size <= 2) {
    if(mov) return PENALTY; //penalize split with no loops
    return gap_size * gap_weight;
  }
  int index = start_index*prb->seq_length + end_index;
  if(in_loop) index = flip_index(index,prb); //in_loop uses bottom of matrix, outter loop uses top of matrix
  if(prb->matrices[mov][index] != EMPTY_VAL) return prb->matrices[mov][index];
  //calculate new entry to get score
  int min = INT_MAX;
  if(mov != RIGHT) min = MIN(min,recurrence_calculator(start_index+1,end_index,mov,in_loop,prb,mdl)+gap_weight);
  if(mov != LEFT) min = MIN(min,recurrence_calculator(start_index,end_index-1,RIGHT,in_loop,prb,mdl)+gap_weight);
  min = MIN(min,recurrence_calculator(start_index+1,end_index-1,FREE,TRUE,prb,mdl) + calc_score(start_index,end_index,prb,mdl));
  if(mov != LEFT) { //left paths not allowed to split
    for(int split_index = start_index+1; split_index < end_index; split_index++) {
      min = MIN(min,recurrence_calculator(start_index,split_index,RIGHT,in_loop,prb,mdl) //only want center empty spaces between loops
                + recurrence_calculator(split_index+1,end_index,LEFT,in_loop,prb,mdl));
    }
  }
  prb->matrices[mov][index] = min;
  return min;
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

void path_finder(int start_index, int end_index, movstate mov, int in_loop, int path_index, problem * prb, model * mdl) {
  int gap_weight = in_loop ? mdl->gweight : 0;
  int gap_size = (end_index - start_index)+1; //end index inclusive
  if(gap_size <= 2) {
    paths[path_index][start_index] = '.';
    paths[path_index][end_index] = '.';
    return;
  }
  int first_match = TRUE;
  int min_score = recurrence_calculator(start_index, end_index, mov, in_loop, prb, mdl);
  if(mov != RIGHT && min_score == recurrence_calculator(start_index+1,end_index,mov,in_loop,prb,mdl)+gap_weight) {
    paths[path_index][start_index] = '.';
    path_finder(start_index+1, end_index, mov, in_loop, path_index, prb, mdl);
    first_match = FALSE;
  }
  if(mov != LEFT && min_score == recurrence_calculator(start_index,end_index-1,RIGHT,in_loop,prb,mdl)+gap_weight) {
    if(!first_match) {
      if(num_paths == MAX_PATHS) return;
      path_index = create_new_path(start_index, end_index, path_index, prb);
    }
    paths[path_index][end_index] = '.';
    path_finder(start_index, end_index-1, RIGHT, in_loop, path_index, prb, mdl);
    first_match = FALSE;
  }
  if(min_score == recurrence_calculator(start_index+1,end_index-1,FREE,TRUE,prb,mdl) + calc_score(start_index,end_index,prb,mdl)) {
    if(!first_match) {
      if(num_paths == MAX_PATHS) return;
      path_index = create_new_path(start_index, end_index, path_index, prb);
    }
    paths[path_index][start_index] = '(';
    paths[path_index][end_index] = ')';
    path_finder(start_index+1, end_index-1, FREE, TRUE, path_index, prb, mdl);
    first_match = FALSE;
  }
  if(mov != LEFT) { //left paths not allowed to split
    for(int split_index = start_index+1; split_index < end_index; split_index++) {
      if(min_score == recurrence_calculator(start_index,split_index,RIGHT,in_loop,prb,mdl) //only want center empty spaces between loops
          + recurrence_calculator(split_index+1,end_index,LEFT,in_loop,prb,mdl))
      {
        if(!first_match) {
          if(num_paths == MAX_PATHS) return;
          path_index = create_new_path(start_index, end_index, path_index, prb);
        }
        first_match = FALSE;
        // count the extra paths so the pairs can be expanded
        int before_paths = num_paths;
        path_finder(start_index, split_index, RIGHT, in_loop, path_index, prb, mdl);
        int left_paths = num_paths - before_paths;
        path_finder(split_index+1, end_index, LEFT, in_loop, path_index, prb, mdl);
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
}


int calc_matrix(problem * prb, model * mdl) {
  return recurrence_calculator(0,prb->seq_length-1,FREE,FALSE,prb,mdl);
}

void find_structures(problem * prb, model * mdl) {
  paths[0] = xmalloc(prb->seq_length+1);
  paths[0][prb->seq_length] = 0;
  num_paths = 1;
  path_finder(0, prb->seq_length-1, FREE, FALSE, 0, prb, mdl);
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
  //TODO: verify alphabet of sequence

  int seq_length = strlen(seq);
  
  printf("I am called with seq %s\n",seq);

#ifdef DEBUG
  printf("Seq is (length=%d): %s\n",seq_length,seq);
#endif

  //clear old path storage
  for(int i=0; i < num_paths; i++) {
    xfree(paths[i]);
  }
  num_paths = 0;
  
  //matrix for storing precomputed values for speedup of recurances
  int grid_size = seq_length*seq_length;
  int * matrix = (int *) xmalloc(sizeof(int)*grid_size*MOVE_MAX);
  for(int i=0; i < grid_size*MOVE_MAX; i++) {
    matrix[i] = EMPTY_VAL;
  }

  //TODO: use a realloc, rather than freeing whole matrix every time
  problem current_problem;
  current_problem.seq = seq;
  current_problem.seq_length = strlen(seq);
  current_problem.matrices[0] = matrix;
  current_problem.matrices[1] = matrix + grid_size;
  current_problem.matrices[2] = matrix + 2*grid_size;
  
  
  min_score = calc_matrix(&current_problem,&current_model);

  find_structures(&current_problem,&current_model);

#ifdef DEBUG
  printf("Min is %d\n", min_score);
  printf("%d\n", num_paths);
#endif

  xfree(matrix);
  return 0;
};

// assumes scaling of 10e-1, TODO: can make scaling dynamic
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
  num_paths = 0;
  const char * seq_pattern = "^[[:space:]]{0,}([AUCG]{1,})";
  if(regcomp(&seq_expr, seq_pattern, REG_EXTENDED) != 0) {
    printf("Aborting all, compiling regex failed\n");
    abort();
  }

#ifndef __EMSCRIPTEN__
  // Put debugging sequence in here to test locally
  return find_minimums("GGCACUGAA");
#else
  return 0;
#endif
}

/* Example Problem With Solution
 *
rshape.js:2034 2, score -2.7
rshape.js:2034 ..(...)..
rshape.js:2034 (...)....
 * */
