#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

int compare( const void *left, const void *right );
void swap( int64_t *arr, unsigned long i, unsigned long j );
unsigned long partition( int64_t *arr, unsigned long start, unsigned long end );
int quicksort( int64_t *arr, unsigned long start, unsigned long end, unsigned long par_threshold );
int quicksort_subprocess( int64_t *arr, unsigned long start, unsigned long end, unsigned long par_threshold );

// TODO: declare additional helper functions if needed

int main( int argc, char **argv ) {
  unsigned long par_threshold;
  if ( argc != 3 || sscanf( argv[2], "%lu", &par_threshold ) != 1 || par_threshold == 0) {
    fprintf( stderr, "Usage: parsort <file> <par threshold>\n" );
    exit( 1 );
  }

  // open the named file
  int fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    // file couldn't be opened: handle error and exit
    fprintf(stderr, "Invalid filename!\n");
    exit( 1 );
  }

  struct stat statbuf;
  int rc = fstat(fd, &statbuf);
  if (rc != 0) {
    // handle fstat error and exit
    fprintf(stderr, "Error with fstat!\n");
    close(fd);
    exit( 1 );
  }

  // determine file size and number of elements
  unsigned long file_size = statbuf.st_size;
  unsigned long num_elements = file_size / sizeof(int64_t);

  // mmap the file data
  int64_t *arr;
  arr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close( fd ); // file can be closed now
  if ( arr == MAP_FAILED ) {
    fprintf(stderr, "Memory mapping failed!\n");
    close(fd);
    exit(1);
  }
  // *arr now behaves like a standard array of int64_t.
  // Be careful though! Going off the end of the array will
  // silently extend the file, which can rapidly lead to
  // disk space depletion!

  // Sort the data!
  int success;
  success = quicksort( arr, 0, num_elements, par_threshold );
  if ( !success ) {
    fprintf( stderr, "Error: sorting failed\n" );
    exit( 1 );
  }

  // Unmap the file data
  munmap(arr, file_size);

  return 0;
}

// Compare elements.
// This function can be used as a comparator for a call to qsort.
//
// Parameters:
//   left - pointer to left element
//   right - pointer to right element
//
// Return:
//   negative if *left < *right,
//   positive if *left > *right,
//   0 if *left == *right
int compare( const void *left, const void *right ) {
  int64_t left_elt = *(const int64_t *)left, right_elt = *(const int64_t *)right;
  if ( left_elt < right_elt )
    return -1;
  else if ( left_elt > right_elt )
    return 1;
  else
    return 0;
}

// Swap array elements.
//
// Parameters:
//   arr - pointer to first element of array
//   i - index of element to swap
//   j - index of other element to swap
void swap( int64_t *arr, unsigned long i, unsigned long j ) {
  int64_t tmp = arr[i];
  arr[i] = arr[j];
  arr[j] = tmp;
}

// Partition a region of given array from start (inclusive)
// to end (exclusive).
//
// Parameters:
//   arr - pointer to first element of array
//   start - inclusive lower bound index
//   end - exclusive upper bound index
//
// Return:
//   index of the pivot element, which is globally in the correct place;
//   all elements to the left of the pivot will have values less than
//   the pivot element, and all elements to the right of the pivot will
//   have values greater than or equal to the pivot
unsigned long partition( int64_t *arr, unsigned long start, unsigned long end ) {
  assert( end > start );

  // choose the middle element as the pivot
  unsigned long len = end - start;
  assert( len >= 2 );
  unsigned long pivot_index = start + (len / 2);
  int64_t pivot_val = arr[pivot_index];

  // stash the pivot at the end of the sequence
  swap( arr, pivot_index, end - 1 );

  // partition all of the elements based on how they compare
  // to the pivot element: elements less than the pivot element
  // should be in the left partition, elements greater than or
  // equal to the pivot should go in the right partition
  unsigned long left_index = start,
                right_index = start + ( len - 2 );

  while ( left_index <= right_index ) {
    // extend the left partition?
    if ( arr[left_index] < pivot_val ) {
      ++left_index;
      continue;
    }

    // extend the right partition?
    if ( arr[right_index] >= pivot_val ) {
      --right_index;
      continue;
    }

    // left_index refers to an element that should be in the right
    // partition, and right_index refers to an element that should
    // be in the left partition, so swap them
    swap( arr, left_index, right_index );
  }

  // at this point, left_index points to the first element
  // in the right partition, so place the pivot element there
  // and return the left index, since that's where the pivot
  // element is now
  swap( arr, left_index, end - 1 );
  return left_index;
}

int wait_handler(pid_t pid) {
  int rc, wstatus;
  rc = waitpid( pid, &wstatus, 0 );
  if ( rc < 0 ) {
    // waitpid failed
    fprintf( stderr, "Waitpid failed!\n" );
    return 0;
  } else {
    // check status of child
    if ( !WIFEXITED( wstatus ) ) {
      // child did not exit normally (e.g., it was terminated by a signal)
      fprintf( stderr, "Child exited abnormally!\n" );
      return 0;
    } else if ( WEXITSTATUS( wstatus ) != 0 ) {
      // child exited with a non-zero exit code
      fprintf( stderr, "Child failed (exited with nonzero exit code)!\n" );
      return 0;
    } else {
      // child exited with exit code zero (it was successful)
      return 1;
    }
  }
}

// Sort specified region of array.
// Note that the only reason that sorting should fail is
// if a child process can't be created or if there is any
// other system call failure.
//
// Parameters:
//   arr - pointer to first element of array
//   start - inclusive lower bound index
//   end - exclusive upper bound index
//   par_threshold - if the number of elements in the region is less
//                   than or equal to this threshold, sort sequentially,
//                   otherwise sort in parallel using child processes
//
// Return:
//   1 if the sort was successful, 0 otherwise
int quicksort( int64_t *arr, unsigned long start, unsigned long end, unsigned long par_threshold ) {
  assert( end >= start );
  unsigned long len = end - start;

  // Base case: if there are fewer than 2 elements to sort,
  // do nothing
  if ( len < 2 ) { return 1; }

  // Base case: if number of elements is less than or equal to
  // the threshold, sort sequentially using qsort
  if ( len <= par_threshold ) {
    qsort( arr + start, len, sizeof(int64_t), compare );
    return 1;
  }

  // Partition
  unsigned long mid = partition( arr, start, end );

  // Recursively sort the left and right partitions
  pid_t left_success, right_success;
  // TODO: modify this code so that the recursive calls execute in child processes
  left_success = quicksort_subprocess( arr, start, mid, par_threshold );
  right_success = quicksort_subprocess( arr, mid + 1, end, par_threshold );

  int l = wait_handler(left_success);
  int r = wait_handler(right_success);

  return l && r;
}

// TODO: define additional helper functions if needed

// Creates a child process to sort half of the array recursively, then
// Forces the parent to wait for the child to conclude before it continues.
//
// Parameters:
//   arr - pointer to first element of array
//   start - inclusive lower bound index
//   end - exclusive upper bound index
//   par_threshold - if the number of elements in the region is less
//                   than or equal to this threshold, sort sequentially,
//                   otherwise sort in parallel using child processes
//
// Return:
//   1 if the child process terminated successfully, 0 otherwise
pid_t quicksort_subprocess( int64_t *arr, unsigned long start, unsigned long end, unsigned long par_threshold ) {
  pid_t child_pid = fork();
  if ( child_pid == 0 ) {
    // executing in the child
    int success = quicksort(arr, start, end, par_threshold);
    if (success) { exit( 0 ); }
    else {exit( 1 ); }
  } else if ( child_pid < 0 ) {
    // fork failed
    fprintf( stderr, "Fork failed!\n" );
    return 0;
  } else {
    return child_pid;
  }
}
