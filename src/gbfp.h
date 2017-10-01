#define LINELEN             65536
#define MEGA                1048576
#define INITGBFSEQNUM       4
#define INITREFERENCENUM    16
#define INITFEATURENUM      64
#define INITQUALIFIERNUM    128
#define FIELDLEN            16
#define FEATURELEN          16
#define QUALIFIERLEN        16
#define LOCUSLEN            16
#define TYPELEN             9
#define TOPOLOGYSTRLEN      8
#define DIVISIONCODELEN     3
#define DATESTRLEN          11
#define QUALIFIERSTART      21

#define INELSE              0
#define INFEATURE           1
#define INQUALIFIER         2

#define NORMAL              'N'
#define REVCOM              'C'
#define LINEAR              'L'
#define CIRCULAR            'C'

#define CHARACTER           'C'
#define LONG                'L'
#define STRING              'S'

typedef char *gb_string;

typedef struct tReference {
    gb_string sAuthors;
    gb_string sConsrtm;
    gb_string sTitle;
    gb_string sJournal;
    gb_string sMedline;
    gb_string sPubMed;
    gb_string sRemark;
    unsigned int iNum;
} gb_reference;

typedef struct tLocation {
    unsigned long lStart;
    unsigned long lEnd;
} gb_location;

typedef struct tQualifier {
    gb_string sQualifier;
    gb_string sValue;
} gb_qualifier;

typedef struct tFeature {
    gb_location *ptLocation;
    gb_qualifier *ptQualifier;
    unsigned long lStart;
    unsigned long lEnd;
    unsigned int iNum;
    unsigned int iLocationNum;
    unsigned int iQualifierNum;
    char sFeature[FEATURELEN + 1];
    char cDirection;
} gb_feature;

typedef struct tGBFFData {
    gb_string sAccession;
    gb_string sComment;
    gb_string sDef;
    gb_string sGI;
    gb_string sKeywords;
    gb_string sLineage;
    gb_string sOrganism;
    gb_string sSequence;
    gb_string sSource;
    gb_string sVersion;
    gb_reference *ptReferences;
    gb_feature *ptFeatures;
    unsigned int iFeatureNum;
    unsigned int iReferenceNum;
    unsigned long lLength;
    unsigned long lRegion[2];
    char sLocusName[LOCUSLEN + 1];
    char sType[TYPELEN + 1];
    char sTopology[TOPOLOGYSTRLEN + 1];
    char sDivisionCode[DIVISIONCODELEN + 1];
    char sDate[DATESTRLEN + 1];
} gb_data;

gb_data **parseGBFF(gb_string spFileName);
void freeGBData(gb_data **pptGBFFData);
gb_string getSequence(gb_string sSequence, gb_feature *ptFeature);
