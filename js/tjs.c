#include "system.h"

#include <rpmio.h>
#include <argv.h>

#define	_RPMJS_INTERNAL
#include <rpmjs.h>

#include <rpmcli.h>

#include "debug.h"

static struct poptOption optionsTable[] = {

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmcliAllPoptTable, 0,
	N_("Common options for all rpm executables:"), NULL },

  POPT_AUTOHELP
  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext optCon = rpmcliInit(argc, argv, optionsTable);
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
    const char * fn;
    const char * str = NULL;
    const char * result;
    int rc = 1;		/* assume failure */

    if (ac < 1) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    while ((fn = *av++) != NULL) {
	rpmRC ret;
	str = rpmExpand("load(\"", fn, "\");", NULL);
	result = NULL;
	if ((ret = rpmjsRun(NULL, str, &result)) != RPMRC_OK)
	    goto exit;
	if (result != NULL && *result != '\0')
	    fprintf(stdout, "%s\n", result);
	str = _free(str);
    }

    rc = 0;

exit:
    str = _free(str);
    optCon = rpmcliFini(optCon);

    return rc;
}