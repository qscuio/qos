#include "frr/xref.h"

struct app_xref {
    struct xref xref;
    int         i;
    char        data[ 128 ];
};

struct app_xrefdata {
    struct xrefdata xrefdata;
    char            data[ 100 ];
};

struct app1_xref {
    struct xref xref;
    int         i;
    char        data[ 128 ];
};

struct app1_xrefdata {
    struct xrefdata xrefdata;
    char            data[ 200 ];
};

struct app2_xref {
    struct xref xref;
    int         i;
    char        data[ 128 ];
};

struct app2_xrefdata {
    struct xrefdata xrefdata;
    char            data[ 300 ];
};

struct app3_xref {
    struct xref xref;
    int         i;
    char        data[ 128 ];
};

struct app3_xrefdata {
    struct xrefdata xrefdata;
    char            data[ 400 ];
};

XREF_SETUP();
int main(int argc, char **argv) { return 0; }
