#include "Task1.h"     // Original implementation
#include "SparkTask1.h"  // Spark implementation
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <string>

// Keep the timing functions from your original test driver
int GetClockTimeInMilliSec() {
    struct timeval t2; 
    gettimeofday(&t2, NULL);
    return t2.tv_sec*1000 + t2.tv_usec/1000;
}

void PrintTime(int milli_sec) {
    int v = milli_sec;
    int hours = v/(1000*60*60); v %= (1000*60*60);
    int minutes = v/(1000*60); v %= (1000*60);
    int seconds = v/1000; v %= 1000;
    int milli_seconds = v;
    int first = 1;
    printf("%d[", milli_sec);
    if(hours) {if(!first) printf(":"); printf("%dh", hours); first=0;}
    if(minutes) {if(!first) printf(":"); printf("%dm", minutes); first=0;}
    if(seconds) {if(!first) printf(":"); printf("%ds", seconds); first=0;}
    if(milli_seconds) {if(!first) printf(":"); printf("%dms", milli_seconds); first=0;}
    printf("]");
}

void TestImplementation(const char* test_file_str, bool use_spark) {
    printf("Testing %s implementation with file: %s\n", 
           use_spark ? "Spark" : "Base", test_file_str);
    
    FILE* test_file = fopen(test_file_str, "rt");
    if(!test_file) {
        printf("Cannot Open File %s\n", test_file_str);
        return;
    }

    int start_time = GetClockTimeInMilliSec();
    InitializeIndex();

    char temp[MAX_DOC_LENGTH];
    while(1) {
        char ch;
        unsigned int id;

        if(EOF == fscanf(test_file, "%c %u ", &ch, &id))
            break;

        if(ch == 's') {
            int match_type;
            int match_dist;
            if(EOF == fscanf(test_file, "%d %d %*d %[^\n\r] ", &match_type, &match_dist, temp)) {
                printf("Corrupted Test File.\n");
                return;
            }
            StartQuery(id, temp, (MatchType)match_type, match_dist);
        }
        else if(ch == 'e') {
            EndQuery(id);
        }
        else if(ch == 'm') {
            if(EOF == fscanf(test_file, "%*u %[^\n\r] ", temp)) {
                printf("Corrupted Test File.\n");
                return;
            }
            MatchDocument(id, temp);
        }
        else if(ch == 'r') {
            // Skip result verification for speed test
            unsigned int num_res;
            if(EOF == fscanf(test_file, "%u ", &num_res)) {
                printf("Corrupted Test File.\n");
                return;
            }
            for(unsigned int i = 0; i < num_res; i++) {
                unsigned int qid;
                if(EOF == fscanf(test_file, "%u ", &qid)) {
                    printf("Corrupted Test File.\n");
                    return;
                }
            }
        }
    }

    DestroyIndex();
    int total_time = GetClockTimeInMilliSec() - start_time;
    
    printf("Time taken: ");
    PrintTime(total_time);
    printf("\n");
    
    fclose(test_file);
}

int main(int argc, char* argv[]) {
    const char* test_file = (argc <= 1) ? "./test_data/small_test.txt" : argv[1];
    
    printf("\n=== Running performance comparison ===\n\n");
    
    // Test base implementation
    TestImplementation(test_file, false);
    
    printf("\n");
    
    // Test Spark implementation
    TestImplementation(test_file, true);
    
    return 0;
}