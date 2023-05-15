
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include<locale.h>

#define ROW (50000/1000)


// Linked list node for words
typedef struct w_node {
    char word[50];
    struct w_node* next;
} Wnode;

// Hash map for words
Wnode* hash_table[1000] = { NULL };


// Array of found words
char found_words[50000][50];//array can store up to 50,000 words, each with a maximum length of 50 characters

int found_words_count = 0;


//Hash index calculation.
int hash(char* word) {
    unsigned long hash = 31;
    for (int i = 0; word[i] != '\0'; i++) {
        hash += word[i];
    }
    return hash % 1000;
}

void print_matrix(int* matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i * cols + j]);
        }
        printf("\n");
    }
}


/// Recursive function to find words in the matrix
int lookup(char* matrix, int* marked, char* word, int pos, int rows, int cols, char** local_found_words, int local_found_words_count) {


    // Base case: word was fully checked (reaching the last character \0) and found.
    if (word[pos] == '\0') {
        // Check if the word we has been found before.
        for (int i = 0; i < local_found_words_count; i++) {
            if (strcmp(local_found_words[i], word) == 0) {
                return 0;
            }
        }
        // Word has not been found before, add to array
        //printf("Thread %d found the word %s\n\n", omp_get_thread_num(), word);
        return 1;
    }

    // Recursive case
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i >= 0 && i < rows && j >= 0 && j < cols && !marked[i * cols + j] && matrix[i * cols + j] == word[pos]) {
                marked[i * cols + j] = 1;  // Mark cell 
                int result = lookup(matrix, marked, word, pos + 1, rows, cols, local_found_words, local_found_words_count);
                if (result == 1) {
                    marked[i * cols + j] = 0;  
                    return 1;
                }
                else {
                        //printf("Thread %d didn't find the word %s\n\n", omp_get_thread_num(), word);
                        marked[i * cols + j] = 0;  
                        return 0; 
                }
                marked[i * cols + j] = 0;  
            }
        }
    }

    return 0;
}


void assign_2_hash(FILE* fp) {

    char word[50];

    while (fscanf_s(fp, "%s", word, 50) != EOF) {
        // Compute hash index and add word to linked list
        int index = hash(word);

        Wnode* node = (Wnode*)malloc(sizeof(Wnode));
        if (node != NULL) {
            memcpy(node->word, word, strlen(word) + 1);
            node->next = hash_table[index];
            hash_table[index] = node;
        }
        else {
            printf("Error: Memory allocation failed\n");
            exit(1);
        }
    }

}

void print_found_words() {
    // Sort words in descending order of length and ascending alphabetical order
    for (int i = 0; i < found_words_count - 1; i++) {
        for (int j = i + 1; j < found_words_count; j++) {
            if (strlen(found_words[i]) < strlen(found_words[j]) || (strlen(found_words[i]) == strlen(found_words[j]) && strcmp(found_words[i], found_words[j]) > 0)) {
                char temp[50];
                strcpy_s(temp, sizeof(temp), found_words[i]);
                strcpy_s(found_words[i], sizeof(found_words[i]), found_words[j]);
                strcpy_s(found_words[j], sizeof(found_words[j]), temp);
            }
        }
    }

    // Group words by length
    int words_length[50] = { 0 }; //each element represents a possible word length
    int max_length = 0;  //  keep track of the maximum length found

    //calculate the length of each word in found_words and save the lengthes greater than 2
    for (int i = 0; i < found_words_count; i++) {
        int word_len = strlen(found_words[i]);
        if (word_len > 2 && word_len < 50) {
            words_length[word_len] = word_len;
            if (word_len > max_length) {
                max_length = word_len;
            }
        }
    }

    // Print words by length in descending order
    for (int len = max_length; len >= 3; len--) {
        if (words_length[len] > 0) {
            printf("Words with %d characters:\n", len);
            for (int i = 0; i < found_words_count; i++) {
                if (strlen(found_words[i]) == len) {
                    printf("%s\n", found_words[i]);
                }
            }
            printf("\n");
        }
    }
}


void print_hash_table() {
    // Set up output stream for UTF-8 encoding
    setlocale(LC_ALL, "en_US.UTF-8");

    // Print the first two indices
    for (int i = 0; i < 2; i++) {
        printf("[%d] ", i);
        Wnode* node = hash_table[i];
        if (node == NULL) {
            printf("NULL\n\n");
        }
        else {
            while (node != NULL) {
                printf("%s->", node->word);
                node = node->next;
            }
            printf("NULL\n\n");
        }
    }

    // Print the last index
    printf("[998] ");
    Wnode* node = hash_table[998];
    if (node == NULL) {
        printf("NULL\n\n");
    }
    else {
        while (node != NULL) {
            printf("%s->", node->word);
            node = node->next;
        }
        printf("NULL\n\n");
    }
}



int main() {

    // Set up output stream for UTF-8 encoding
    setlocale(LC_ALL, "Turkish");

    //open .ini file
    FILE* iniFile;
    char line[100];
    int rows, cols;

    errno_t error = fopen_s(&iniFile, "matrix.ini", "r");
    if (error != 0)
    {
        printf("Error: unable to open file!\n");
        return 1;
    }

    //get dimentions from .ini file
    while (fgets(line, sizeof(line), iniFile))
    {
        if (sscanf_s(line, "row=%d", &rows) == 1)
            continue;  // Move to the next line

        if (sscanf_s(line, "col=%d", &cols) == 1)
            break;  // We found the row and column values, so we can exit the loop

        // If we reach here, the line is not a valid row or column value
        printf("Error: invalid line in ini file\n");
        return 1;
    }


    fclose(iniFile);

    //initialize character matrix with the dimentions.

    char* matrix = (char*)malloc(rows * cols * sizeof(char));
    int* marked = (int*)calloc(rows * cols, sizeof(int));


    printf("Processing matrix with dimensions %dx%d\n", rows, cols);

    //assign random characters to the matrix and insure to include some of these characters. (insure getting an output)
    char* characters = (char*)"kmkicosoaacknmdpğçöü";

    int num_characters = strlen(characters);

    srand(time(NULL));
    for (int i = 0; i < rows * cols; i++) {
        if (i < num_characters) {
            // Use characters from the string until we run out
            matrix[i] = characters[i];
        }
        else {
            // Generate a random letter
            matrix[i] = 'a' + rand() % 26;
        }
    }


    // Print 2D matrix
    printf("2D matrix of random characters:\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%c ", matrix[i * cols + j]);//access individual elements using 2D indexing: arr[0 * 4 + 0] = equivalent to arr[0][0]
        }
        printf("\n");
    }


    // Open file and load words into hash map
    FILE* fp;
    fopen_s(&fp, "words.txt", "r");

    assign_2_hash(fp);

    fclose(fp);


    double start_time, end_time;



    // Start parallel timer
    start_time = omp_get_wtime();



    int num_th = 4;
    // Run program in parallel
    //the parallel loop indices are evenly divided among the threads at the start of the loop.
    //result in each thread executing the block of parallel for which includes calling the
    //lookup function for a chunk of hash indices to find words in those.
    //using a reduction clause might not be the best approach since it will result in a private copy of the entire array for each thread,
   
#pragma omp parallel num_threads(num_th)

    {//initialize variables for each thread that will hold the local found words and the count of local found words
        int local_found_words_count = 0;
        int max_local_found_words = 100; // initial allocation size
        char** local_found_words = (char**)malloc(max_local_found_words * sizeof(char*));;//a pointer to a dynamically allocated array of characters with a size of 50

        for (int i = 0; i < max_local_found_words; i++) {
            local_found_words[i] = (char*)malloc(50 * sizeof(char));
        }
        
        #pragma omp for schedule(static)
        for (int i = 0; i < 1000; i++) {
            Wnode* node = hash_table[i];
            while (node != NULL) {
                if (node->word != NULL) {
                    //the loop searches for each word in the matrix and, if found, adds it to the local local_found_words array.
                    //If the local_found_words array is not large enough to hold all the found words, it will be resized using realloc.
                    if (lookup(matrix, marked, node->word, 0, rows, cols, local_found_words, local_found_words_count)) {

                        // resize array if necessary
                        //For example, if the local_found_words_count is 200 and the initial size of local_found_words array is 100,
                        //the condition local_found_words_count >= max_local_found_words will be true, and the size of local_found_words
                        //array will be doubled to 200. Then, the for loop will allocate memory for the new elements from index 100 to 199
                        if (local_found_words_count >= max_local_found_words) {
                            max_local_found_words *= 2; //Double the size and reallcoate the needed memroy
                            local_found_words = (char**)realloc(local_found_words, max_local_found_words * sizeof(char*));

                            for (int j = max_local_found_words / 2; j < max_local_found_words; j++) {
                                local_found_words[j] = (char*)malloc(50 * sizeof(char));
                            }
                        }
                        //Add the words found within the thread into its local found words.
                        strcpy_s(local_found_words[local_found_words_count], 50, node->word);
                        local_found_words_count++;
                    }
                    node = node->next;
                }
            }
        }
        //Each thread is copying its local_found_words array to the end of the global found_words array in
        //a critical section, which ensures that no two threads access the shared array simultaneously.
        #pragma omp critical
        {
            for (int i = 0; i < local_found_words_count; i++) {
                strcpy_s(found_words[found_words_count], 50, local_found_words[i]);
                found_words_count++;
            }
        }

        // free memory allocated within thread locally.
        for (int i = 0; i < max_local_found_words; i++) {
            free(local_found_words[i]);
        }
        free(local_found_words);
    }



    // End parallel timer
    end_time = omp_get_wtime();


    // Calculate parallel speedup and efficiency
    double time_taken = end_time - start_time;

    // Open file for writing results
    FILE* results_file;
    errno_t err = fopen_s(&results_file, "results.csv", "a+"); // open in read+write mode to count lines and write to the end of the file
    if (err != 0) {
        printf("Error: Failed to open file\n");
        exit(1);
    }
    setlocale(LC_ALL, "en_US");

    // Write results to file
    fprintf(results_file, "%d,%d,%d,%2lf\n", num_th, rows, cols, time_taken);

    fclose(results_file);

    print_found_words();
    printf("time taken: %lf", time_taken);
    print_hash_table();


    return 0;
}




//https://github.com/olaDEN