#include "SparkTask1.h"
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////

int GetClockTimeInMilliSec()
{
    struct timeval t2; gettimeofday(&t2,NULL);
    return t2.tv_sec*1000+t2.tv_usec/1000;
}

void PrintTime(int milli_sec)
{
    int v=milli_sec;
    int hours=v/(1000*60*60); v%=(1000*60*60);
    int minutes=v/(1000*60); v%=(1000*60);
    int seconds=v/1000; v%=1000;
    int milli_seconds=v;
    int first=1;
    printf("%d[", milli_sec);
    if(hours) {if(!first) printf(":"); printf("%dh", hours); first=0;}
    if(minutes) {if(!first) printf(":"); printf("%dm", minutes); first=0;}
    if(seconds) {if(!first) printf(":"); printf("%ds", seconds); first=0;}
    if(milli_seconds) {if(!first) printf(":"); printf("%dms", milli_seconds); first=0;}
    printf("]");
}

///////////////////////////////////////////////////////////////////////////////////////////////

char temp[MAX_DOC_LENGTH];

void TestSpark(const char* test_file_str)
{
    int i, j;
    printf("Start Spark Test ...\n"); fflush(NULL);
    FILE* test_file=fopen(test_file_str, "rt");

    if(!test_file)
    {
        printf("Cannot Open File %s\n", test_file_str);
        fflush(NULL);
        return;
    }

    int v=GetClockTimeInMilliSec();
    InitializeIndex();

    unsigned int first_result=0;
    int num_cur_results=0;

    const int max_results=100;

    bool cur_results_ret[max_results];
    unsigned int cur_results_size[max_results];
    unsigned int* cur_results[max_results];

    while(1)
    {
        char ch;
        unsigned int id;

        if(EOF==fscanf(test_file, "%c %u ", &ch, &id))
            break;

        if(num_cur_results && (ch=='s' || ch=='e'))
        {
            for(i=0;i<num_cur_results;i++)
            {
                unsigned int doc_id=0;
                unsigned int num_res=0;
                unsigned int* query_ids=0;

                ErrorCode err=GetNextAvailRes(&doc_id, &num_res, &query_ids);

                if(err==EC_NO_AVAIL_RES)
                {
                    printf("The call to GetNextAvailRes() returned EC_NO_AVAIL_RES, but there is still undelivered documents.\n");
                    fflush(NULL);
                    return;
                }
                else if(err==EC_FAIL)
                {
                    printf("The call to GetNextAvailRes() returned EC_FAIL.\n");
                    fflush(NULL);
                    return;
                }
                else if(err!=EC_SUCCESS)
                {
                    printf("The call to GetNextAvailRes() returned unknown error code.\n");
                    fflush(NULL);
                    return;
                }

                if(doc_id<first_result || doc_id-first_result>=(unsigned int)num_cur_results)
                {
                    printf("The call to GetNextAvailRes() returned unknown document ID %u.\n", doc_id);
                    fflush(NULL);
                    return;
                }
                if(cur_results_ret[doc_id-first_result])
                {
                    printf("The call to GetNextAvailRes() returned document (ID=%u) that has been delivered before.\n", doc_id);
                    fflush(NULL);
                    return;
                }

                bool flag_error=false;

                if(num_res!=cur_results_size[doc_id-first_result])
                {
                    flag_error=true;
                }

                for(j=0;j<(int)num_res && !flag_error;j++)
                {
                    if(query_ids[j]!=cur_results[doc_id-first_result][j])
                    {
                        flag_error=true;
                    }
                }

                if(flag_error)
                {
                    printf("The call to GetNextAvailRes() returned incorrect result for document ID %u.\n", doc_id);
                    printf("Your answer is: "); for(j=0;j<(int)num_res;j++) {if(j)printf(" "); printf("%u", query_ids[j]);} printf("\n");
                    printf("The correct answer is: "); for(j=0;j<(int)cur_results_size[doc_id-first_result];j++) {if(j)printf(" "); printf("%u", cur_results[doc_id-first_result][j]);} printf("\n");
                    fflush(NULL);
                    return;
                }

                cur_results_ret[doc_id-first_result]=true;
                if(num_res && query_ids) free(query_ids);
            }

            for(i=0;i<num_cur_results;i++) {free(cur_results[i]); cur_results[i]=0; cur_results_size[i]=0; cur_results_ret[i]=false;}
            num_cur_results=0;
        }

        if(ch=='s')
        {
            int match_type;
            int match_dist;

            if(EOF==fscanf(test_file, "%d %d %*d %[^\n\r] ", &match_type, &match_dist, temp))
            {
                printf("Corrupted Test File.\n");
                fflush(NULL);
                return;
            }
            
            ErrorCode err=StartQuery(id, temp, (MatchType)match_type, match_dist);

            if(err==EC_FAIL)
            {
                printf("The call to StartQuery() returned EC_FAIL.\n");
                fflush(NULL);
                return;
            }
            else if(err!=EC_SUCCESS)
            {
                printf("The call to StartQuery() returned unknown error code.\n");
                fflush(NULL);
                return;
            }
        }
        else if(ch=='e')
        {
            ErrorCode err=EndQuery(id);

            if(err==EC_FAIL)
            {
                printf("The call to EndQuery() returned EC_FAIL.\n");
                fflush(NULL);
                return;
            }
            else if(err!=EC_SUCCESS)
            {
                printf("The call to EndQuery() returned unknown error code.\n");
                fflush(NULL);
                return;
            }
        }
        else if(ch=='m')
        {
            if(EOF==fscanf(test_file, "%*u %[^\n\r] ", temp))
            {
                printf("Corrupted Test File.\n");
                fflush(NULL);
                return;
            }

            ErrorCode err=MatchDocument(id, temp);

            if(err==EC_FAIL)
            {
                printf("The call to MatchDocument() returned EC_FAIL.\n");
                fflush(NULL);
                return;
            }
            else if(err!=EC_SUCCESS)
            {
                printf("The call to MatchDocument() returned unknown error code.\n");
                fflush(NULL);
                return;
            }
        }
        else if(ch=='r')
        {
            unsigned int num_res=0;
            if(EOF==fscanf(test_file, "%u ", &num_res))
            {
                printf("Corrupted Test File.\n");
                fflush(NULL);
                return;
            }
            
            if(num_cur_results==0) first_result=id;
            unsigned int qid;
            cur_results_ret[num_cur_results]=false;
            cur_results_size[num_cur_results]=num_res;
            cur_results[num_cur_results]=(unsigned int*)malloc(num_res*sizeof(unsigned int));
            
            for(i=0;i<(int)num_res;i++)
            {
                if(EOF==fscanf(test_file, "%u ", &qid))
                {
                    printf("Corrupted Test File.\n");
                    fflush(NULL);
                    return;
                }
                
                cur_results[num_cur_results][i]=qid;
            }
            num_cur_results++;
        }
        else
        {
            printf("Corrupted Test File. Unknown Command %c.\n", ch);
            fflush(NULL);
            return;
        }
    }

    v=GetClockTimeInMilliSec()-v;

    DestroyIndex();

    fclose(test_file);

    printf("Spark implementation has successfully passed all tests.\n");
    printf("Time="); PrintTime(v); printf("\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    if(argc<=1) TestSpark("./test_data/small_test.txt");
    else TestSpark(argv[1]);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////