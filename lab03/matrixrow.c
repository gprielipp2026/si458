#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <xmmintrin.h>
#include <immintrin.h>

/**
 * Outline:
 * 1. parse argv:
 *  1. <path mat1> <path mat2> <?path debug mat>
 * 2. parse file:
 *  1. format:
 *    rows cols
 *    value value value .... // row 1
 *    value value value .... // row 2
 *    ... // to EOF
 * 3. print A * B (mat1 * mat2)
 * 4. cleanup memory
 */

typedef struct {
  bool isRowForm;
  int rows, cols;
  float** data;
} mat_t;

typedef struct {
  // using pointers so memory is not copied between function calls -> only the pointer value is
  mat_t *matA, *matB; // required
  mat_t *debug; // optional
} info_t;

info_t* parse_args(int argc, char* argv[]);
void free_info(info_t* info);
void free_mat(mat_t* mat, bool freePtr);
mat_t* read_file(char* path);
mat_t* mul(mat_t* A, mat_t* B);
void mat_print(mat_t* mat);
bool debug(info_t* info, mat_t* C);

// ------------------------ main ------------------------
__attribute__ ((no_instrument_function))
int main(int argc, char* argv[]) {
  info_t* info = parse_args(argc, argv);
  
  mat_t* C = mul(info->matA, info->matB);

  if(info->debug != NULL) {
    printf( debug(info, C) ? "passed\n" : "failed\n" );
  } else {
    mat_print(C);
  }
  
  free_mat(C, true); 

  free_info(info);
}

// ------------------------ main ------------------------

__attribute__((no_instrument_function))
info_t* parse_args(int argc, char* argv[]) 
{
  if(argc != 3 && argc != 4) {
    printf("usage: %s <path to matrix 1> <path to matrix 2> <?path to debug matrix>\n", argv[0]);
    exit(0);
  }

  info_t* info = malloc(sizeof(*info));

  mat_t* mat;
  if( (mat = read_file(argv[1])) == NULL) {
    free_info(info);
    exit(0);
  }


  info->matA = mat;
  
  if( (mat = read_file(argv[2])) == NULL) {
    free_info(info);
    exit(0);
  }

  info->matB = mat;

  if(argc == 4) {
    if( (mat = read_file(argv[3])) == NULL) {
      free_info(info);
      exit(0);
    }
    info->debug = mat;
  }
  else info->debug = NULL;

  return info;
}

  __attribute__((no_instrument_function))
void free_info(info_t* info)
{
  free_mat(info->matA, true);
  free_mat(info->matB, true);
  free_mat(info->debug, true);
  free(info);
}

  __attribute__((no_instrument_function))
void free_mat(mat_t* mat, bool freePtr)
{
  if(mat != NULL) {
    // loop over it efficiently
    if(mat->isRowForm) {
      for(int row = 0; row < mat->rows; row++) {
        free(mat->data[row]);
      }
    }
    else {
      for(int col = 0; col < mat->cols; col++) {
        free(mat->data[col]);
      }
    }
    free(mat->data);
    if(freePtr) free(mat);
  }
}

__attribute__((no_instrument_function))
mat_t* read_file(char* path)
{
  FILE* file = fopen(path, "r");
  if(!file) {
    fprintf(stderr, "Failed to open '%s'\n", path);
    exit(1);
  }
  
  mat_t* mat = malloc(sizeof(*mat));
  
  fscanf(file, "%d %d", &mat->rows, &mat->cols);
  
  // change this
  mat->isRowForm = true;

  if(mat->isRowForm) {
    mat->data = malloc(sizeof(float*)*mat->rows);
    for(int row = 0; row < mat->rows; row++) {
      mat->data[row] = malloc(sizeof(float)*mat->cols); 
      for(int col = 0; col < mat->cols; col++) {
        fscanf(file, "%f", &mat->data[row][col]); 
      } 
    }
  }
  else {
    mat->data = malloc(sizeof(float*)*mat->cols);
    for(int col = 0; col < mat->cols; col++) {
      mat->data[col] = malloc(sizeof(float)*mat->rows); 
      for(int row = 0; row < mat->rows; row++) { 
        fscanf(file, "%f", &mat->data[col][row]); 
      }
    }
  } 

  fclose(file);

  return mat;
}


// ------------------- multiply matrices ---------------
__attribute__((no_instrument_function))
void swap_row_col_form(mat_t* mat) 
{
  mat->isRowForm = !mat->isRowForm;
  int N = (mat->isRowForm ? mat->rows:mat->cols); 
  int M = (mat->isRowForm ? mat->cols:mat->rows); 
 
  float** tmp= malloc(sizeof(float*)*N);

  for(int i = 0; i < N; i++) {
    tmp[i] = malloc(sizeof(float)*M);
    for(int j = 0; j < M; j++) {
      tmp[i][j] = mat->data[j][i];
    }
  }
  
  free_mat(mat, false);

  mat->data = tmp;
}

mat_t* mul(mat_t* A, mat_t* B)
{
  if(!A->isRowForm) swap_row_col_form(A);
  if(!B->isRowForm) swap_row_col_form(B);

  if(A->rows != B->cols) {
    printf("Invalid matrix multiplication of %dx%d * %dx%d\n", A->cols, A->rows, B->cols, B->rows);
    return NULL;
  }

  // make the new matrix
  mat_t* C = malloc(sizeof(*C));
  C->cols = A->cols;
  C->rows = B->rows;
  C->isRowForm = true;

  C->data = malloc(sizeof(float*)*C->rows);

  // multiply the matrices
  for(int row = 0; row < C->rows; row++) {
    C->data[row] = malloc(sizeof(float)*C->cols);
    
    // just assume M % 4 == 0 for NxK * KxM
    for(int col = 0; col < C->cols; col+=4) {
      __m128 spotSum = _mm_setzero_ps();

      for(int k = 0; k < A->rows; k++) {
        __m128 vrow = _mm_broadcast_ss(&A->data[row][k]);
        __m128 vcol = _mm_load_ps(&B->data[k][col]);
        __m128 vres = _mm_mul_ps(vrow, vcol);
        spotSum = _mm_add_ps(spotSum, vres);
      }

      // save the result
      _mm_store_ps(&C->data[row][col], spotSum);
    }
  }

  return C;
}

// -----------------------------------------------------


  __attribute__((no_instrument_function))
void mat_print(mat_t* mat)
{
  if(mat->isRowForm) {
    for(int row = 0; row < mat->rows; row++) {
      for(int col = 0; col < mat->cols; col++) {
        printf("%d ", (int)mat->data[row][col]);
      }
      printf("\n");
    }
  }
  else {
    for(int row = 0; row < mat->rows; row++) {
      for(int col = 0; col < mat->cols; col++) {
        printf("%d ", (int)mat->data[col][row]);
      }
      printf("\n");
    }
  }

}

  __attribute__((no_instrument_function))
bool debug(info_t* info, mat_t* C)
{
  if(info->debug->isRowForm != C->isRowForm) swap_row_col_form(C);
  int N = (C->isRowForm ? C->rows:C->cols); 
  int M = (C->isRowForm ? C->cols:C->rows); 

  for(int i = 0; i < N; i++) {
    for(int j = 0; j < M; j++) {
      if(info->debug->data[i][j] != C->data[i][j]) return false;
    }
  }

  return true;
}

