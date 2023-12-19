#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include "gbfp.h"
#include "countrycodes.h"

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

void help(void) {
        printf("Extract from a GenBank flat file.\n"
        "\n"
        "Usage: gbmunge [-h] -i <Genbank_file> -f <sequence_output> -o <metadata_output> [-t] [-s]\n"
        "\n");
}

static char *getQualValue(char *sQualifier, gb_feature *ptFeature) {
    gb_qualifier *i;
    for (i = ptFeature->ptQualifier; (i - ptFeature->ptQualifier) < ptFeature->iQualifierNum; i++)
        if (strcmp(sQualifier, i->sQualifier) == 0)
            return i->sValue;
    return NULL;
}

int minIndex(int *a, int n){
    if(n <= 0) return -1;
    int i, min_i = 0;
    int min = a[0];
    for(i=1;i<n;++i){
        if(a[i]<min){
            min = a[i];
            min_i = i;
        }
    }
    return min_i;
}

int levenshteinDistance(char *s1, char *s2) {
   // https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C
    unsigned int x, y, s1len, s2len;
    s1len = strlen(s1);
    s2len = strlen(s2);
    unsigned int matrix[s2len+1][s1len+1];
    matrix[0][0] = 0;
    for (x = 1; x <= s2len; x++)
        matrix[x][0] = matrix[x-1][0] + 1;
    for (y = 1; y <= s1len; y++)
        matrix[0][y] = matrix[0][y-1] + 1;
    for (x = 1; x <= s2len; x++)
        for (y = 1; y <= s1len; y++)
            matrix[x][y] = MIN3(matrix[x-1][y] + 1, matrix[x][y-1] + 1, matrix[x-1][y-1] + (s1[y-1] == s2[x-1] ? 0 : 1));

    return(matrix[s2len][s1len]);
}

int compareStrings(char *s1, char *s2)
{
for (; *s1 && *s2 && (toupper(*s1) == toupper(*s2)); ++s1, ++s2);
return *s1 - *s2;
}

double diceMatch(const char *string1, const char *string2) {
    // https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Dice%27s_coefficient#C
    if (((string1 != NULL) && (string1[0] == '\0')) ||
        ((string2 != NULL) && (string2[0] == '\0'))) {
        return 0;
    }
    if (string1 == string2) {
        return 1;
    }

    size_t strlen1 = strlen(string1);
    size_t strlen2 = strlen(string2);
    if (strlen1 < 2 || strlen2 < 2) {
        return 0;
    }

    size_t length1 = strlen1 - 1;
    size_t length2 = strlen2 - 1;

    double matches = 0;
    size_t i = 0, j = 0;

    while (i < length1 && j < length2) {
        char a[3] = {string1[i], string1[i + 1], '\0'};
        char b[3] = {string2[j], string2[j + 1], '\0'};
        int cmp = compareStrings(a, b);
        if (cmp == 0) {
            matches += 2;
        }
        i++;
        j++;
    }

    return matches / (length1 + length2);
}

char* uppercase ( char *sPtr )
{
    char *sCopy = malloc (1 + strlen (sPtr));
    int i;
    if (sCopy == NULL) return NULL;
    strcpy(sCopy,sPtr);
    for(i=0; sCopy[i] != '\0'; i++){
        /* Check if character in inputArray is lower Case*/
        if(islower(sCopy[i])){
            /* Convert lower case character to upper case
              using toupper function */
            sCopy[i] = toupper(sCopy[i]);
        } else {
            sCopy[i] = sCopy[i];
        }
    }
    sCopy[i] = '\0';
    return(sCopy);
}

int main(int argc, char *argv[]) {
    char *sFileName = NULL;
    char *sFasta = NULL;
    char *sTable = NULL;
    char *sDate = NULL;
    char *sCountry = NULL;
    char *sCountry2 = NULL;
    char *sHost = NULL;
    int sNoMissingDates = 0;
    int sIncludeSequence = 0;
    struct tm ltm = {0};
    struct tm cltm = {0};
    char sDate2[] = "0001-01-01";
    char sCollectionDate[] = "0001-01-01";
    int ld[NUM_COUNTRY];

    gb_data **pptSeqData, *ptSeqData;
    gb_feature *ptFeature;
    size_t i,j,k,idx;

    FILE *fFasta;
    FILE *fTable;

    int iOpt;
    while((iOpt = getopt(argc, argv, "h:i:f:o:ts")) != -1) {
     switch(iOpt) {
     case 'h':
         help();
         exit(0);
         break;
     case 'i':
         sFileName = optarg;
         break;
     case 'f':
         sFasta = optarg;
         break;
     case 'o':
         sTable = optarg;
         break;
     case 't':
         sNoMissingDates = 1;
         break;
     case 's':
         sIncludeSequence = 1;
         break;
     default:
         help();
         exit(0);
     }
    }

    if(sFileName == NULL){
        printf("%s","Error: No input filename specified.\n\n");
        help();
        exit(0);
    }

    if(sFasta == NULL){
        printf("%s","Error: No FASTA filename specified.\n\n");
        help();
        exit(0);
    }

    if(sTable == NULL){
        printf("%s","Error: No output filename specified.\n\n");
        help();
        exit(0);
    }

    pptSeqData = parseGBFF(sFileName); /* parse a GBF file which contains more than one GBF sequence data */
    fFasta = fopen(sFasta,"w");
    fTable = fopen(sTable,"w");
    fprintf(fTable,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
    "name",
    "accession",
    "length",
    "submission_date",
    "host",
    "country_original",
    "country",
    "countrycode",
    "collection_original",
    "collection_date");
    if(sIncludeSequence==1){
     fprintf(fTable,"\t%s","sequence");
    }
    fprintf(fTable,"\n");
    for (i = 0; (ptSeqData = *(pptSeqData + i)) != NULL; i++) { /* ptSeqData points a parsed data of a GBF sequence data */
      for (j = 0; j < ptSeqData->iFeatureNum; j++) {
            ptFeature = (ptSeqData->ptFeatures + j);
            if (strcmp("source", ptFeature->sFeature) == 0) {
                sDate = getQualValue("collection_date",ptFeature);
                if(sDate!=NULL){
                    if(strlen(sDate)==11){
                        strptime(sDate, "%d-%b-%Y", &ltm);
                        strftime(sDate2, sizeof(sDate2), "%Y-%m-%d", &ltm);
                    }
                    if(strlen(sDate)==10){
                        if(strncmp(&sDate[4],"-",1)==0){
                          strptime(sDate, "%Y-%m-%d", &ltm);
                        }else{
                          strptime(sDate, "%d-%m-%Y", &ltm);
                        }
                        strftime(sDate2, sizeof(sDate2), "%Y-%m-%d", &ltm);
                    }
                    if(strlen(sDate)==8){
                        if(strncmp(&sDate[4],"-",1)==0){
                          strptime(sDate, "%Y-%b", &ltm);
                        }
                        else{
                          strptime(sDate, "%b-%Y", &ltm);
                        }
                        strftime(sDate2, sizeof(sDate2), "%Y-%m", &ltm);
                    }
                    if(strlen(sDate)==7){
                        if(strncmp(&sDate[4],"-",1)==0){
                          strptime(sDate, "%Y-%m", &ltm);
                        }else{
                          strptime(sDate, "%m-%Y", &ltm);
                        }
                        strftime(sDate2, sizeof(sDate2), "%Y-%m", &ltm);
                    }
                    if(strlen(sDate)==4){
                        strptime(sDate, "%Y", &ltm);
                        strftime(sDate2, sizeof(sDate2), "%Y", &ltm);
                    }
                }
                sHost = getQualValue("host",ptFeature);
                if(sHost!=NULL){
                    sHost = strtok(sHost,";");
                }
                sCountry = getQualValue("country",ptFeature);
                if(sCountry!=NULL){
                    sCountry2 = malloc(1+strlen(sCountry));
	                strcpy(sCountry2,sCountry);
                    sCountry2 = strtok(sCountry2,":");
                    for (k=0;k < NUM_COUNTRY;k++){
                        ld[k] = levenshteinDistance(sCountry2,country[k]);
                    }
                    idx = minIndex(ld,NUM_COUNTRY);
                }
                }
            }
      strptime(ptSeqData->sDate,"%d-%b-%Y",&cltm);
      strftime(sCollectionDate, sizeof(sCollectionDate),"%Y-%m-%d", &cltm);
      if(sNoMissingDates==1){
        if(sDate != NULL){
        fprintf(fFasta,">%s_%s\n%s\n",ptSeqData->sAccession,sDate2,ptSeqData->sSequence);
        fprintf(fTable,"%s_%s\t%s\t%lu\t%s\t%s\t%s\t%s\t%s\t%s\t%s%s%s\n",
        ptSeqData->sAccession,
        sDate2,
        ptSeqData->sAccession,
        ptSeqData->lLength,
        sCollectionDate,
        sHost == NULL ? "NA" : sHost,
        sCountry == NULL ? "NA" : sCountry,
        sCountry == NULL ? "NA" : country[idx],
        sCountry == NULL ? "NA" : countrycode[idx],
        sDate,
        sDate2,
        sIncludeSequence == 0 ? "" : "\t",
        sIncludeSequence == 0 ? "" : ptSeqData->sSequence
        );
        }
      }
      else{
        fprintf(fFasta,">%s\n%s\n",ptSeqData->sAccession,ptSeqData->sSequence);
        fprintf(fTable,"%s\t%s\t%lu\t%s\t%s\t%s\t%s\t%s\t%s\t%s%s%s\n",
        ptSeqData->sAccession,
        ptSeqData->sAccession,
        ptSeqData->lLength,
        sCollectionDate,
        sHost == NULL ? "NA" : sHost,
        sCountry == NULL ? "NA" : sCountry,
        sCountry == NULL ? "NA" : country[idx],
        sCountry == NULL ? "NA" : countrycode[idx],
        sDate == NULL ? "NA" : sDate,
        sDate == NULL ? "NA" : sDate2,
        sIncludeSequence == 0 ? "" : "\t",
        sIncludeSequence == 0 ? "" : ptSeqData->sSequence
        );
        }
    }
    freeGBData(pptSeqData); /* release memory space */
    fclose(fTable);
    fclose(fFasta);
    return 0;
}
