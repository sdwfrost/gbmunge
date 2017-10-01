#define __EXTENSIONS__

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
 
#include "gbfp.h"

const char sVer[] = "0.6.1";
const char sNorBase[] = "ACGTRYMKWSBDHVNacgtrymkwsbdhvn";
const char sComBase[] = "TGCAYRKMWSVHDBNtgcayrkmwsvhdbn";
const unsigned int iBaseLen = 30;
char sTempLine[LINELEN] = {'\0',};
regex_t ptRegExLocus;
regex_t ptRegExOneLine;
regex_t ptRegExAccession;
regex_t ptRegExVersion;
regex_t ptRegExRegion;
regex_t ptRegExGI;

#define skipSpace( x ) for (; isspace(*x); x++)
#define putLine( x ) strcpy(sTempLine, x)
#define getLine_w_rtrim( x, y ) \
    getLine(x, y); \
    rtrim(x)

/* Initializes regular expression */
void initRegEx(void) {
    const char sLocus[] = "^LOCUS +([a-z|A-Z|0-9|_]+) +([0-9]+) bp +([a-z|A-Z|\-| ]+) ([a-z| ]{8}) ([A-Z| ]{3}) ([0-9]+-[A-Z]+-[0-9]+)";
    const char sOneLine[] = "^ *([A-Z]+) +(.+)";
    const char sAccession[] = "^ACCESSION +([a-z|A-Z|0-9|_]+) ?";
    const char sRegion[] = " +REGION: ?([0-9]+)\\.\\.([0-9]+)";
    const char sVersion[] = "^VERSION +([a-z|A-Z|0-9|_.]+) ?";
    const char sGI[] = " +GI: ?([0-9]+)";
 
    regcomp(&ptRegExLocus, sLocus, REG_EXTENDED | REG_ICASE);
    regcomp(&ptRegExOneLine, sOneLine, REG_EXTENDED | REG_ICASE);
    regcomp(&ptRegExAccession, sAccession, REG_EXTENDED | REG_ICASE);
    regcomp(&ptRegExVersion, sVersion, REG_EXTENDED | REG_ICASE);
    regcomp(&ptRegExRegion, sRegion, REG_EXTENDED | REG_ICASE);
    regcomp(&ptRegExGI, sGI, REG_EXTENDED | REG_ICASE);
}

void freeRegEx(void) {
    regfree(&ptRegExLocus);
    regfree(&ptRegExOneLine);
    regfree(&ptRegExAccession);
    regfree(&ptRegExVersion);
    regfree(&ptRegExRegion);
    regfree(&ptRegExGI);
}

/* Removes white spaces at end of a string */
static void rtrim(gb_string sLine) {
    register int i;
    
    for (i = (strlen(sLine) - 1); i >= 0; i--) if (! isspace(*(sLine + i))) break;
    *(sLine + i + 1) = '\0';
}

/* Removes a specific character at end of a string */
static void removeRChar(gb_string sLine, char cRemove) {
    register int i;
    
    for (i = (strlen(sLine) - 1); i >= 0; i--) {
        if (sLine[i] == cRemove) {
            sLine[i] = '\0';
            break;
        }
    }
}

/* Gets a line from either the line buffer or the file */
static gb_string getLine(gb_string sLine, FILE *FSeqFile) {
    gb_string sReturn;

    if (*sTempLine != '\0') {
        sReturn = strcpy(sLine, sTempLine);
        *sTempLine = '\0';
    } else {
        sReturn = fgets(sLine, LINELEN, FSeqFile);
    }

    return sReturn;
}

/* Concatenates lines which start with specific white spaces */
static gb_string joinLines(FILE *FSeqFile, unsigned int iSpaceLen) {
    char sLine[LINELEN];
    gb_string sTemp, sJoinedLine;

    sJoinedLine = malloc(sizeof(char) * LINELEN);

    getLine_w_rtrim(sLine, FSeqFile);
    strcpy(sJoinedLine, sLine + iSpaceLen);

    while (fgets(sLine, LINELEN, FSeqFile)) {
        sTemp = sLine;
        skipSpace(sTemp);
        if ((sTemp - sLine) < iSpaceLen) break;
        rtrim(sTemp);
        sJoinedLine = strcat(sJoinedLine, sTemp - 1); /* '- 1' in order to insert a space character at the juncation */
    }

    putLine(sLine);

    return realloc(sJoinedLine, sizeof(char) * (strlen(sJoinedLine) + 1));
}

static int parseLocus(gb_string sLocusStr, gb_data *ptGBData) {
    /*    
    01-05      'LOCUS'
    06-12      spaces
    13-28      Locus name
    29-29      space
    30-40      Length of sequence, right-justified
    41-41      space
    42-43      bp
    44-44      space
    45-47      spaces, ss- (single-stranded), ds- (double-stranded), or
               ms- (mixed-stranded)
    48-53      NA, DNA, RNA, tRNA (transfer RNA), rRNA (ribosomal RNA),
               mRNA (messenger RNA), uRNA (small nuclear RNA), snRNA,
               snoRNA. Left justified.
    54-55      space
    56-63      'linear' followed by two spaces, or 'circular'
    64-64      space
    65-67      The division code (see Section 3.3)
    68-68      space
    69-79      Date, in the form dd-MMM-yyyy (e.g., 15-MAR-1991)
    */
    
    char sTemp[LINELEN];
    unsigned int i, iErr, iLen;
    
    regmatch_t ptRegMatch[7];
    
    struct tData {
        char cType;
        void *Pointer;
    } tDatas[] = {
        {STRING, NULL},
        {LONG, NULL},
        {STRING, NULL},
        {STRING, NULL},
        {STRING, NULL},
        {STRING, NULL}};

    tDatas[0].Pointer = ptGBData->sLocusName;
    tDatas[1].Pointer = &(ptGBData->lLength);
    tDatas[2].Pointer = ptGBData->sType;
    tDatas[3].Pointer = ptGBData->sTopology;
    tDatas[4].Pointer = ptGBData->sDivisionCode;
    tDatas[5].Pointer = ptGBData->sDate;
    
    rtrim(sLocusStr);
        
    if ((iErr = regexec(&ptRegExLocus, sLocusStr, 7, ptRegMatch, 0)) == 0) {
        for (i = 0; i < 6; i++) {
            iLen = ptRegMatch[i + 1].rm_eo - ptRegMatch[i + 1].rm_so;
            switch (tDatas[i].cType) {
            case STRING:
                memcpy(tDatas[i].Pointer, (sLocusStr + ptRegMatch[i + 1].rm_so), iLen);
                *((gb_string) tDatas[i].Pointer + iLen) = '\0';
                rtrim((gb_string) tDatas[i].Pointer);
                break;
            case LONG:
                memcpy(sTemp, (sLocusStr + ptRegMatch[i + 1].rm_so), iLen);
                sTemp[iLen] = '\0';
                *((unsigned long *) tDatas[i].Pointer) = atol(sTemp);
                break;
            default:
                perror("Unknown Data Type!");
            }
        }
    } else {
        /* regerror(iErr, &ptRegExLocus, sTemp, LINELEN); */
        /* perror("Invalid LOCUS line!"); */
        fprintf(stderr, "Invalid LOCUS line! - '%s\n'", sLocusStr);
        return 1;
    }
    return 0;
}

static void parseDef(FILE *FSeqFile, gb_data *ptGBData) {
    char sLine[LINELEN];
    regmatch_t ptRegMatch[3];

    getLine_w_rtrim(sLine, FSeqFile);

    regexec(&ptRegExOneLine, sLine, 3, ptRegMatch, 0);
    ptGBData->sDef = strdup(sLine + ptRegMatch[2].rm_so);
}

static void parseKeywords(FILE *FSeqFile, gb_data *ptGBData) {
    char sLine[LINELEN];
    regmatch_t ptRegMatch[3];

    getLine_w_rtrim(sLine, FSeqFile);

    regexec(&ptRegExOneLine, sLine, 3, ptRegMatch, 0);
    ptGBData->sKeywords = strdup(sLine + ptRegMatch[2].rm_so);
}

static void parseAccession(FILE *FSeqFile, gb_data *ptGBData) {
    char sLine[LINELEN];
    regmatch_t ptRegMatch[3];

    getLine_w_rtrim(sLine, FSeqFile);

    if (regexec(&ptRegExAccession, sLine, 2, ptRegMatch, 0) == 0) {
        *(sLine + ptRegMatch[1].rm_eo) = '\0';
        ptGBData->sAccession = strdup(sLine + ptRegMatch[1].rm_so);
    }

    if (regexec(&ptRegExRegion, sLine + ptRegMatch[1].rm_eo + 1, 3, ptRegMatch, 0) == 0) {
        *(sLine + ptRegMatch[1].rm_eo) = '\0';
        (ptGBData->lRegion)[0] = atol(sLine + ptRegMatch[1].rm_so);
        *(sLine + ptRegMatch[2].rm_eo) = '\0';
        (ptGBData->lRegion)[1] = atol(sLine + ptRegMatch[2].rm_so);
    }
}

static void parseVersion(FILE *FSeqFile, gb_data *ptGBData) {
    char sLine[LINELEN];
    regmatch_t ptRegMatch[2];

    getLine_w_rtrim(sLine, FSeqFile);

    if (regexec(&ptRegExVersion, sLine, 2, ptRegMatch, 0) == 0) {
        *(sLine + ptRegMatch[1].rm_eo) = '\0';
        ptGBData->sVersion = strdup(sLine + ptRegMatch[1].rm_so);
    }
    if (regexec(&ptRegExGI, sLine + ptRegMatch[1].rm_eo + 1, 2, ptRegMatch, 0) == 0) {
        *(sLine + ptRegMatch[1].rm_eo) = '\0';
        ptGBData->sGI = strdup(sLine + ptRegMatch[1].rm_so);
    }
}

static void parseComment(FILE *FSeqFile, gb_data *ptGBData) {
    ptGBData->sComment = joinLines(FSeqFile, 12);
}

static void parseSource(FILE *FSeqFile, gb_data *ptGBData) {
    char sLine[LINELEN];
    regmatch_t ptRegMatch[3];

    getLine_w_rtrim(sLine, FSeqFile);
    regexec(&ptRegExOneLine, sLine, 3, ptRegMatch, 0);
    ptGBData->sSource = strdup(sLine + ptRegMatch[2].rm_so);

    getLine_w_rtrim(sLine, FSeqFile);
    regexec(&ptRegExOneLine, sLine, 3, ptRegMatch, 0);
    ptGBData->sOrganism = strdup(sLine + ptRegMatch[2].rm_so);

    ptGBData->sLineage = joinLines(FSeqFile, 12);
}

#define processRef( x, y ) \
    y = NULL; \
    getLine_w_rtrim(sLine, FSeqFile); \
    putLine(sLine); \
    if (strstr(sLine, x) != NULL) y = joinLines(FSeqFile, 12)

static void parseReference(FILE *FSeqFile, gb_data *ptGBData) {
    char sLine[LINELEN];
    regmatch_t ptRegMatch[3];
    gb_reference *ptReferences = NULL;
    gb_reference *ptReference = NULL;
    unsigned int iReferenceNum = 0;

    ptReferences = ptGBData->ptReferences;
    iReferenceNum = ptGBData->iReferenceNum;

    ptReferences = realloc(ptReferences, sizeof(gb_reference) * (iReferenceNum + 1));
    ptReference = ptReferences + iReferenceNum;

    getLine_w_rtrim(sLine, FSeqFile);
    regexec(&ptRegExOneLine, sLine, 3, ptRegMatch, 0);
    ptReference->iNum = atoi(sLine + ptRegMatch[2].rm_so);

    processRef("  AUTHORS  ", ptReference->sAuthors);
    processRef("  CONSRTM  ", ptReference->sConsrtm);
    processRef("  TITLE  ",   ptReference->sTitle);
    processRef("  JOURNAL  ", ptReference->sJournal);
    processRef("  MEDLINE  ", ptReference->sMedline);
    processRef("  PUBMED  ",  ptReference->sPubMed);
    processRef("  REMARK  ",  ptReference->sRemark);

    ptGBData->ptReferences = ptReferences;
    ptGBData->iReferenceNum = iReferenceNum + 1;
}

static gb_string checkComplement(gb_string sLocation) {
    gb_string sPosition;

    skipSpace(sLocation);

    for (sPosition = sLocation; *sPosition; sPosition++) {
        /* Check the 1st and the 2nd characters of 'complement' */
        if (*sPosition == 'c' && *(sPosition + 1) == 'o') {
            removeRChar(sLocation, ')');
            return sPosition + 11;
        }
    }

    return sLocation;
}

static gb_string checkJoin(gb_string sLocation) {
    gb_string sPosition;

    skipSpace(sLocation);

    for (sPosition = sLocation; *sPosition; sPosition++) {
        /* Check the 1st and the 2nd characters of 'complement' */
        if (*sPosition == 'j' && *(sPosition + 1) == 'o') {
            removeRChar(sLocation, ')');
            return sPosition + 5;
        }
    }

    return sLocation;
}

static int convertPos2Num(gb_string sPositions, unsigned long *lStart, unsigned long *lEnd) {
    register int i;
    int aiPositions[4] = {-2,};
    int iNum = 0;

    for (i = strlen(sPositions); i >= 0; i--) {
        if (isdigit(*(sPositions + i))) aiPositions[(aiPositions[iNum] - 1 == i) ? iNum : ++iNum] = i;
        else *(sPositions + i) = '\0';
    }
   
    if (iNum == 2) {
        *lStart = atol(sPositions + aiPositions[2]);
        *lEnd = atol(sPositions + aiPositions[1]);

        return 1;
    } else if (iNum == 1) {
        *lStart = *lEnd = atol(sPositions + aiPositions[1]);
       
        return 1;
    } else {
        fprintf(stderr, "Warning: cannot parse '%s'\n", sPositions);

        return 0;
    }

}

/* Parsing a gb_string that contains gb_location information */
static void parseLocation(gb_string sLocation, gb_feature *pFeature) {
    gb_string sTemp;
    gb_string sString = NULL;
    
    unsigned int iLocationNum = 1;
    
    /* Evalue sequence direction
    sString has gb_location and join informations
    */

    sString = checkComplement(sLocation);

    if (sLocation == sString) pFeature->cDirection = NORMAL;
    else pFeature->cDirection = REVCOM;

    /* Remove 'join' gb_string
    sString has gb_location informations
    */

    sString = checkJoin(sString);

    sTemp = sString - 1;
    while((sTemp = strchr((sTemp + 1), ','))) iLocationNum++;
    pFeature->ptLocation = malloc(iLocationNum * sizeof(*(pFeature->ptLocation)));

    iLocationNum = 0;
    sLocation = strtok_r(sString, ",", &sTemp);
    if (convertPos2Num(sLocation,
        &(((pFeature->ptLocation)+iLocationNum)->lStart),
        &(((pFeature->ptLocation)+iLocationNum)->lEnd)) == 1) iLocationNum++;

    while((sLocation = strtok_r(NULL, ",", &sTemp))) {
        if (convertPos2Num(sLocation,
            &(((pFeature->ptLocation)+iLocationNum)->lStart),
            &(((pFeature->ptLocation)+iLocationNum)->lEnd)) == 1) iLocationNum++;
    }
    
    pFeature->lStart = (pFeature->ptLocation)->lStart;
    pFeature->lEnd = ((pFeature->ptLocation)+(iLocationNum - 1))->lEnd;
    pFeature->iLocationNum = iLocationNum;
}

static gb_string _parseQualifier(gb_string sQualifier, gb_string *psValue) {
    gb_string sPosition;

    skipSpace(sQualifier);

    if ((sPosition = strchr(sQualifier, '=')) == NULL) {
        *psValue = sQualifier + strlen(sQualifier);
        return sQualifier;
    }

    *sPosition++ = '\0';

    skipSpace(sQualifier);

    if (*sPosition == '"') removeRChar(++sPosition, '"');

    *psValue = sPosition;

    return sQualifier;
}

static void parseQualifier(gb_string sQualifier, gb_feature *pFeature) {
    gb_string sValue;
    gb_string sTemp = NULL;
    gb_string sString = NULL;
    gb_qualifier *ptQualifier;
   
    ptQualifier = malloc(INITQUALIFIERNUM * sizeof(gb_qualifier));
    pFeature->ptQualifier = ptQualifier;
    

    /* Parse the 1st gb_qualifier gb_string */
    sString = strtok_r(sQualifier, "\n", &sTemp);
    ptQualifier->sQualifier = _parseQualifier(sString, &sValue);
    ptQualifier->sValue = sValue;
    ptQualifier++;
    
    /* Parse the rest gb_qualifier gb_string */
    while((sString = strtok_r(NULL, "\n", &sTemp)) != NULL) {
        ptQualifier->sQualifier = _parseQualifier(sString, &sValue);
        ptQualifier->sValue = sValue;
        ptQualifier++;
    }

    /* Determine the number of actual qualifier data */
    pFeature->iQualifierNum = ptQualifier - pFeature->ptQualifier;
    pFeature->ptQualifier =  realloc(pFeature->ptQualifier, pFeature->iQualifierNum * sizeof(gb_qualifier));
    /*
    ptQualifier = malloc(pFeature->iQualifierNum * sizeof(gb_qualifier));
    memcpy(ptQualifier, pFeature->ptQualifier, pFeature->iQualifierNum * sizeof(gb_qualifier));
    free(pFeature->ptQualifier);
    pFeature->ptQualifier = ptQualifier;
    */
}

static void parseFeature(FILE *FSeqFile, gb_data *ptGBData) {
    char sLine[LINELEN] = {'\0',};
    char sLocation[LINELEN] = {'\0',};
    gb_string sQualifier = NULL;
    gb_string sQualifierTemp = NULL;
    unsigned int iReadPos = INELSE;
    unsigned int iFeatureNum = 0;
    unsigned int iFeatureMem = INITFEATURENUM;
    unsigned int i = 0;
    gb_feature *pFeatures = NULL;
    gb_feature *pFeature = NULL;
    
    pFeatures = (gb_feature *) malloc(iFeatureMem * sizeof(gb_feature));

    /* Parse FEATURES */
    while(fgets(sLine, LINELEN, FSeqFile)) {
        if (! isspace(*sLine)) {
            putLine(sLine);
            break;
        }
        
        rtrim(sLine);
        if (memcmp(sLine + 5, "               ", 15) != 0) {
            if (iFeatureNum == iFeatureMem) {
                iFeatureMem += INITFEATURENUM;
                pFeatures = realloc(pFeatures, sizeof(gb_feature) * iFeatureMem);
            }

            if (strlen(sLocation) != 0) parseLocation(sLocation, (pFeatures + iFeatureNum - 1));
            if (sQualifier < sQualifierTemp) {
                *sQualifierTemp++ = '\n';
                *sQualifierTemp = '\0';
                sQualifierTemp = malloc((sQualifierTemp - sQualifier + 1) * sizeof(*sQualifier));
                strcpy(sQualifierTemp, sQualifier);
                free(sQualifier);
                sQualifier = sQualifierTemp;
                parseQualifier(sQualifier, (pFeatures + iFeatureNum - 1));
            } else {
                free(sQualifier);
                sQualifier = NULL;
            }

            *sLocation = '\0';
            sQualifier = malloc(sizeof(*sQualifier) * MEGA);
            sQualifierTemp = sQualifier;
            
            iReadPos = INFEATURE;
            
            memcpy((pFeatures + iFeatureNum)->sFeature, (sLine + 5), 15);
            *(((pFeatures + iFeatureNum)->sFeature) + 15) = '\0';
            rtrim((pFeatures + iFeatureNum)->sFeature);
            strcpy(sLocation, (sLine + 21));

            /* Feature Initalize */
            pFeature = pFeatures + iFeatureNum;
            pFeature->iNum = iFeatureNum;
            pFeature->cDirection = NORMAL;
            pFeature->iLocationNum = 0;
            pFeature->lStart = 0;
            pFeature->lEnd = 0;
            pFeature->iQualifierNum = 0;
            pFeature->ptLocation = NULL;
            pFeature->ptQualifier = NULL;

            iFeatureNum++;
        } else if (*(sLine + QUALIFIERSTART) == '/') {
            iReadPos = INQUALIFIER;
            if (sQualifier < sQualifierTemp) *sQualifierTemp++ = '\n';
            i = strlen(sLine) - (QUALIFIERSTART + 1);
            memcpy(sQualifierTemp, sLine + (QUALIFIERSTART + 1), i);
            sQualifierTemp += i;
        } else {
            if (iReadPos == INFEATURE) {
                strcpy((sLocation + strlen(sLocation)), (sLine + QUALIFIERSTART));
            } else if (iReadPos == INQUALIFIER) {
                i = strlen(sLine) - QUALIFIERSTART;
                memcpy(sQualifierTemp, sLine + QUALIFIERSTART, i);
                sQualifierTemp += i;
            }
        }
    }

    /* Finishing of the parsing */

    if (iFeatureNum == iFeatureMem) {
        iFeatureMem += INITFEATURENUM;
        pFeatures = realloc(pFeatures, sizeof(gb_feature) * iFeatureMem);
    }

    if (strlen(sLocation) != 0) parseLocation(sLocation, (pFeatures + iFeatureNum - 1));
    if (sQualifier < sQualifierTemp) {
        *sQualifierTemp++ = '\n';
        *sQualifierTemp = '\0';
        sQualifierTemp = malloc((sQualifierTemp - sQualifier + 1) * sizeof(*sQualifier));
        strcpy(sQualifierTemp, sQualifier);
        free(sQualifier);
        sQualifier = sQualifierTemp;
        parseQualifier(sQualifier, (pFeatures + iFeatureNum - 1));
    } else {
        free(sQualifier);
        sQualifier = NULL;
    }

    ptGBData->iFeatureNum = iFeatureNum;
    ptGBData->ptFeatures = pFeatures;
}

/* Parse sequences */
static void parseSequence(FILE *FSeqFile, gb_data *ptGBData) {
    register char c;
    char sLine[LINELEN] = {'\0',};
    gb_string sSequence, sSequence2;

    ptGBData->sSequence = malloc((ptGBData->lLength + 1) * sizeof(char));
    sSequence2 = ptGBData->sSequence;
    
    while(fgets(sLine, LINELEN, FSeqFile)) {
        if (*sLine == '/' && *(sLine + 1) == '/') {
            putLine(sLine);
            break;
        }
        sSequence = sLine + 9; /* '+ 9' in order to skip a numbers */
        while((c = *(sSequence++)) != '\0') if (isalpha(c)) *(sSequence2++) = c;
    }
    *(sSequence2) = '\0';
}

static void initGBData(gb_data *ptGBData) {
    ptGBData->sAccession = NULL;
    ptGBData->sComment = NULL;
    ptGBData->sDef = NULL;
    ptGBData->sGI = NULL;
    ptGBData->sKeywords = NULL;
    ptGBData->sLineage = NULL;
    ptGBData->sOrganism = NULL;
    ptGBData->sSequence = NULL;
    ptGBData->sSource = NULL;
    ptGBData->sVersion = NULL;
    ptGBData->ptReferences = NULL;
    ptGBData->ptFeatures = NULL;
    ptGBData->iFeatureNum = 0;
    ptGBData->iReferenceNum = 0;
    ptGBData->lLength = 0;
    ptGBData->lRegion[0] = 0;
    ptGBData->lRegion[1] = 0;
    ptGBData->sLocusName[0] = '\0';
    ptGBData->sType[0] = '\0';
    ptGBData->sTopology[0] = '\0';
    ptGBData->sDivisionCode[0] = '\0';
    ptGBData->sDate[0] = '\0';
}

static gb_data *_parseGBFF(FILE *FSeqFile) {
    int i;
    char sLine[LINELEN] = {'\0',};
    gb_data *ptGBData = NULL;

    struct tField {
        char sField[FIELDLEN + 1];
        void (*vFunction)(FILE *FSeqFile, gb_data *ptGBData);
    } atFields[] = {
        {"DEFINITION", parseDef},
        {"ACCESSION", parseAccession},
        {"VERSION", parseVersion},
        {"KEYWORDS", parseKeywords},
        {"SOURCE", parseSource},
        {"REFERENCE", parseReference},
        {"COMMENT", parseComment},
        {"FEATURE", parseFeature},
        {"ORIGIN", parseSequence},
        {"", NULL} /* To terminate seeking */
    };

    /* Confirming GBFF File with LOCUS line */
    while(fgets(sLine, LINELEN, FSeqFile)) {
        if (strstr(sLine, "LOCUS") == sLine) {
            ptGBData = malloc(sizeof(gb_data));
            initGBData(ptGBData);
            break;
        }
    }

    /* If there is a no LOCUS line, next statement return NULL value to end parsing */
    if (ptGBData == NULL) return NULL;
   
    /* Parse LOCUS line */ 
    if (parseLocus(sLine, ptGBData) != 0) {
        free(ptGBData);
        return NULL;
    }

    while(getLine(sLine, FSeqFile)) {
        if (*sLine == '/' && *(sLine + 1) == '/') break;
        for(i = 0; *((atFields + i)->sField); i++) {
            if (strstr(sLine, (atFields + i)->sField) == sLine) {
                putLine(sLine);
                ((atFields + i)->vFunction)(FSeqFile, ptGBData);
                break;
            }
        }
    }
    
    return ptGBData;
}

/* parse sequence datas in a GBF file */
gb_data **parseGBFF(gb_string spFileName) {
    int iGBFSeqPos = 0;
    unsigned int iGBFSeqNum = INITGBFSEQNUM;
    gb_data **pptGBDatas;
    FILE *FSeqFile;
 
    if (spFileName == NULL) {
        FSeqFile = stdin;
    } else {
        if (access(spFileName, F_OK) != 0) {
            /* perror(spFileName); */
            return NULL;
        } else {
            FSeqFile = fopen(spFileName, "r");
        }
    }
   
    initRegEx(); /* Initalize for regular expression */

    pptGBDatas = malloc(iGBFSeqNum * sizeof(gb_data *));

    do {
        if (iGBFSeqNum == iGBFSeqPos) {
            iGBFSeqNum += INITGBFSEQNUM;
            pptGBDatas = realloc(pptGBDatas, iGBFSeqNum * sizeof(gb_data *));
        }
        *(pptGBDatas + iGBFSeqPos) = _parseGBFF(FSeqFile);
    } while (*(pptGBDatas + iGBFSeqPos++) != NULL);
    
    if (spFileName) fclose(FSeqFile);

    freeRegEx();

    return pptGBDatas;
}

void freeGBData(gb_data **pptGBData) {
    int i;
    gb_data *ptGBData = NULL;
    gb_feature *ptFeatures = NULL;
    gb_reference *ptReferences = NULL;
    unsigned int iFeatureNum = 0;
    unsigned int iReferenceNum = 0;
    unsigned int iSeqPos = 0;
  
    for (iSeqPos = 0; *(pptGBData + iSeqPos) != NULL; iSeqPos++) {
        ptGBData = *(pptGBData + iSeqPos);

        ptFeatures = ptGBData->ptFeatures;
        iFeatureNum = ptGBData->iFeatureNum;


        /* Release memory space for features */    
        for (i = 0; i < iFeatureNum; i++) {
            free((ptFeatures + i)->ptLocation);
            if ((ptFeatures + i)->ptQualifier != NULL) {
                free(((ptFeatures + i)->ptQualifier)->sQualifier);
                free((ptFeatures + i)->ptQualifier);
            }
        }

        free(ptFeatures);

        /* Release memory space for References */
        ptReferences = ptGBData->ptReferences;
        iReferenceNum = ptGBData->iReferenceNum;
        for (i = 0; i < iReferenceNum; i++) {
            free((ptReferences + i)->sAuthors);
            free((ptReferences + i)->sConsrtm);
            free((ptReferences + i)->sTitle);
            free((ptReferences + i)->sJournal);
            free((ptReferences + i)->sMedline);
            free((ptReferences + i)->sPubMed);
            free((ptReferences + i)->sRemark);
        }
        free(ptReferences);

        free(ptGBData->sDef);
        free(ptGBData->sAccession);
        free(ptGBData->sComment);
        free(ptGBData->sGI);
        free(ptGBData->sKeywords);
        free(ptGBData->sLineage);
        free(ptGBData->sOrganism);
        free(ptGBData->sSequence);
        free(ptGBData->sSource);
        free(ptGBData->sVersion);

        free(ptGBData);
    }

    free(pptGBData);
}

static void getRevCom(gb_string sSequence) {
    char c;
    unsigned int k;
    unsigned long i, j;
    
    for (i = 0, j = strlen(sSequence) - 1; i < j; i++, j--) {
	c = *(sSequence + i);
	*(sSequence + i) = 'X';
	for (k = 0; k < iBaseLen; k++)
	    if (*(sNorBase + k) == *(sSequence + j)) {
		*(sSequence + i) = *(sComBase + k);
		break;
	    }
	*(sSequence + j) = 'X';
	for (k = 0; k < iBaseLen; k++)
	    if (*(sNorBase + k) == c) {
		*(sSequence + j) = *(sComBase + k);
		break;
	}
    }
}

gb_string getSequence(gb_string sSequence, gb_feature *ptFeature) {
    unsigned long lSeqLen = 1; /* For the '\0' characher */
    unsigned long lStart, lEnd;
    unsigned int i;
    gb_string sSequenceTemp;
    
    for (i = 0; i < ptFeature->iLocationNum; i++)
        lSeqLen += (((ptFeature->ptLocation) + i)->lEnd - ((ptFeature->ptLocation) + i)->lStart + 1);
    
    sSequenceTemp = malloc(lSeqLen * sizeof(char));
    
    lSeqLen = 0;
    
    for (i = 0; i < ptFeature->iLocationNum; i++) {
        lStart = ((ptFeature->ptLocation) + i)->lStart;
        lEnd = ((ptFeature->ptLocation) + i)->lEnd;
        strncpy(sSequenceTemp + lSeqLen, sSequence + lStart - 1, lEnd - lStart + 1);
        lSeqLen += (lEnd - lStart + 1);
    }
    
    *(sSequenceTemp + lSeqLen) = '\0';
    
    if (ptFeature->cDirection == REVCOM) getRevCom(sSequenceTemp);

    return sSequenceTemp;
}

