
#include "system.h"

#if defined(WITH_READLINE)
#if defined(HAVE_READLINE_READLINE_H)
#include <readline/readline.h>
#endif
#if defined(HAVE_READLINE_HISTORY_H)
#include <readline/history.h>
#endif
#endif

#define	_RPMIOB_INTERNAL
#include <rpmiotypes.h>
#include <rpmlog.h>
#include <poptIO.h>

#define	_RPMSM_INTERNAL
#include <rpmsm.h>

#include "debug.h"

#define F_ISSET(_sm, _FLAG) (((_sm)->flags & ((RPMSM_FLAGS_##_FLAG) & ~0x40000000)) != RPMSM_FLAGS_NONE)

/* forward reference */
extern const struct rpmioC_s const _rpmsmCommands[];

static char *cleanstr(char *path, const char sep)
{
    if (path && *path) {
	char *e = path + strlen(path) - 1;
	while (e >= path && (*e == sep || xisspace(*e)))
	    *e-- = '\0';
    }
    return path;
}

static int cmd_quit(int ac, char *av[])
{
    exit(EXIT_SUCCESS);		/* XXX FIXME: quit is useless for embedded */
}

static int cmd_run(int ac, char *av[])
{
    char * cmd = argvJoin((ARGV_t)av, ' ');
    const char * cav[] = { cmd, NULL };
    int rc;
    if (!strcmp(av[0], "reload") || !strcmp(av[0], "build"))
	*cmd = xtoupper(*cmd);
    rc = rpmsmRun(NULL, cav, NULL);
    cmd = _free(cmd);
    return rc;
}

static int cmd_help(int ac, /*@unused@*/ char *av[])
{
    FILE * fp = stdout;
    rpmioC c;

    fprintf(fp, "Commands:\n\n");
    for (c = (rpmioC)_rpmsmCommands; c->name != NULL; c++) {
        fprintf(fp, "    %s\n        %s\n\n", c->synopsis, c->help);
    }
    return 0;
}

const struct rpmioC_s const _rpmsmCommands[] = {
    { "exit",  0, 0, cmd_quit, "exit",
      "Exit the program."
    },
    { "quit",  0, 0, cmd_quit, "quit",
      "Exit the program."
    },
    { "list",  0, 1, cmd_run, "list [REGEX]",
      "List installed policy modules that match REGEX."
    },

    { "base",  1, 1, cmd_run, "base FILE",
      "Install a new base policy module FILE."
    },
    { "install",  1, 1, cmd_run, "install FILE",
      "Install a new policy module FILE."
    },
    { "upgrade",  1, 1, cmd_run, "upgrade FILE",
      "Upgrade an existing policy module FILE."
    },
    { "remove",  1, 1, cmd_run, "remove MODULE",
      "Remove an existing policy MODULE."
    },
    { "reload",  0, 1, cmd_run, "reload",
      "Reload policy."
    },
    { "build",  0, 1, cmd_run, "build",
      "Build and reload policy."
    },

    { "help", 0, 0, cmd_help, "help",
      "Print this help text"
    },
    { NULL, -1, -1, NULL, NULL, NULL }
};

static rpmRC _rpmsmRun(rpmsm sm, const char * str, const char ** resultp)
{
    rpmioP P = NULL;
    rpmRC rc = RPMRC_OK;	/* assume success */
    int xx;

    if (sm == NULL) sm = _rpmsmI;

    if (resultp)
	*resultp = NULL;

    while (rpmioParse(&P, str) != RPMRC_NOTFOUND) {	/* XXX exit on EOS */
	rpmioC c;
	str = NULL;

	if (P->av && P->ac > 0 && P->av[0] != NULL && strlen(P->av[0]) > 0) {

	    for (c = (rpmioC) _rpmsmCommands; c->name; c++) {
	        if (!strcmp(P->av[0], c->name))
	            break;
	    }
	    if (c->name == NULL) {
		rpmiobAppend(sm->iob, "Unknown command '", 0);
		rpmiobAppend(sm->iob, P->av[0], 0);
		rpmiobAppend(sm->iob, "'\n", 0);
		rc = RPMRC_FAIL;
	    } else
	    if ((P->ac - 1) < c->minargs) {
		rpmiobAppend(sm->iob, "Not enough arguments for ", 0);
		rpmiobAppend(sm->iob, c->name, 0);
		rpmiobAppend(sm->iob, "\n", 0);
		rc = RPMRC_FAIL;
	    } else
	    if ((P->ac - 1) > c->maxargs) {
		rpmiobAppend(sm->iob, "Too many arguments for ", 0);
		rpmiobAppend(sm->iob, c->name, 0);
		rpmiobAppend(sm->iob, "\n", 0);
		rc = RPMRC_FAIL;
	    } else
	    if ((xx = (*c->handler)(P->ac, (char **)P->av)) < 0) {
		static char ibuf[32];
		(void) snprintf(ibuf, sizeof(ibuf), "%d", xx);
		rpmiobAppend(sm->iob, "Failed(", 0);
		rpmiobAppend(sm->iob, ibuf, 0);
		rpmiobAppend(sm->iob, "): ", 0);
		rpmiobAppend(sm->iob, P->av[0], 0);
		rpmiobAppend(sm->iob, "\n", 0);
		rc = RPMRC_FAIL;
	    }
	}
	if (rc != RPMRC_OK)
	    break;
    }

    if (sm != NULL) {
	rpmiob iob = sm->iob;
	if (resultp && iob->blen > 0) /* XXX return result iff bytes appended */
	    *resultp = rpmiobStr(iob);
	iob->blen = 0;			/* XXX reset the print buffer */
    }

    rpmioPFree(P);

    return rc;
}

static int main_loop(void)
{
    char *line = NULL;
    size_t len = 0;
    int ret = 0;	/* assume success */

    while (1) {
	const char *buf;

#if defined(WITH_READLINE)
        if (isatty(fileno(stdin))) {
            line = readline("boo> ");
        } else
#endif
	if (getline(&line, &len, stdin) == -1)
	    break;
        cleanstr(line, '\n');
        if (line == NULL) {
            fprintf(stdout, "\n");
	    break;
        }
        if (line[0] == '#')
            continue;

	buf = NULL;
	if (_rpmsmRun(NULL, line, &buf) == RPMRC_OK) {
#if defined(WITH_READLINE)
	    if (isatty(fileno(stdin)))
		add_history(line);
#endif
	}
	if (buf && *buf) {
	    const char * eol = (buf[strlen(buf)-1] != '\n' ? "\n" : "");
	    fprintf(stdout, "%s%s", buf, eol);
	}
    }
    line = _free(line);
    return ret;
}

/**
 */
static void rpmsmArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
        /*@globals _rpmsm, h_errno, fileSystem, internalState @*/
        /*@modifies _rpmsm, fileSystem, internalState @*/
{
    rpmsm sm = _rpmsmI;
    char * t = NULL;
    int xx;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case 'b':
	sm->flags |= RPMSM_FLAGS_BASE;
	sm->flags |= RPMSM_FLAGS_CREATE;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'i':
	sm->flags |= RPMSM_FLAGS_INSTALL;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'l':
	sm->flags |= RPMSM_FLAGS_LIST;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'r':
	sm->flags |= RPMSM_FLAGS_REMOVE;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'u':
	sm->flags |= RPMSM_FLAGS_UPGRADE;
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'R':
	sm->flags |= RPMSM_FLAGS_RELOAD;
	arg = "";	/* XXX no arg for --reload */
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'n':
	sm->flags &= ~RPMSM_FLAGS_RELOAD;
	break;
    case 's':
	sm->fn = _free(sm->fn);
	sm->fn = xstrdup(arg);
	break;
    case 'B':
	sm->flags |= RPMSM_FLAGS_REBUILD;
	arg = "";	/* XXX no arg for --build */
	t = rpmExpand("  ", arg, NULL); *t = opt->val;
	xx = argvAdd(&sm->av, t);
	break;
    case 'D':
	sm->flags |= RPMSM_FLAGS_NOAUDIT;
	break;

    case 'h':
    case '?':
    default:
	fprintf(stderr, _("%s: Unknown option -%c\n"), __progname, opt->val);
	poptPrintUsage(con, stderr, 0);
	exit(EXIT_FAILURE);
	/*@notreached@*/ break;
    }
    t = _free(t);
}

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmsmOtherTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmsmArgCallback, 0, NULL, NULL },
/*@=type@*/
  { "store",'s', POPT_ARG_STRING,		NULL, (int)'s',
	N_("Set the STORE to operate on (e.g. \"targeted\")"), N_("STORE") },
  { "noreload",'n', POPT_ARG_NONE,		NULL, (int)'n',
	N_("Do not reload policy after commit"), NULL },
  { "disable_dontaudit",'D', POPT_ARG_NONE,	NULL, (int)'D',
	N_("Remove dontaudits from policy"), NULL },
  POPT_TABLEEND
};

/*@unchecked@*/ /*@observer@*/
static struct poptOption rpmsmOptionsTable[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
        rpmsmArgCallback, 0, NULL, NULL },
/*@=type@*/

  { "list",'l', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL,	NULL, (int)'l',
	N_("Display list of installed policy modules"), N_("REGEX") },
  { "install",'i', POPT_ARG_STRING,			NULL, (int)'i',
	N_("Install a new module FILE"), N_("FILE") },
  { "upgrade",'u', POPT_ARG_STRING,			NULL, (int)'u',
	N_("Upgrade an existing module FILE"), N_("FILE") },
  { "base",'b', POPT_ARG_STRING,			NULL, (int)'b',
	N_("Install a new base module FILE"), N_("FILE") },
  { "remove",'r', POPT_ARG_STRING,			NULL, (int)'r',
	N_("Remove an existing MODULE"), N_("MODULE") },
  { "reload",'R', POPT_ARG_NONE,			NULL, (int)'R',
	N_("Reload policy"), NULL },
  { "build",'B', POPT_ARG_NONE,				NULL, (int)'B',
	N_("Build and reload policy"), NULL },

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, rpmsmOtherTable, 0,
	N_("Other options:"), NULL },

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"),
	NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP
  POPT_TABLEEND
};

int main(int argc, char *argv[])
{
    rpmsm sm = NULL;
    poptContext optCon = NULL;
    const char ** av = NULL;
    const char * result = NULL;
    int rc = -1;	/* assume failure */

    __progname = "semodule";
    sm = rpmsmNew(NULL, 0);
    if (sm == NULL) {
	fprintf(stderr, "%s:  Could not create handle\n", __progname);
	goto exit;
    }
    _rpmsmI = sm;

    /* Parse CLI options and args. */
    optCon = rpmioInit(argc, argv, rpmsmOptionsTable);
    av = poptGetArgs(optCon);

    if (!(av && *av) || !strcmp(av[0], "-")) {
	rc = main_loop();
	goto exit;
    }

    if (av && *av) {
	int ac = argvCount(av);
	char lcmd = (av && ac > 0 ? (*av)[0] : 0);
	int i;

	switch (lcmd) {
	default:
	    fprintf(stderr, "unknown additional arguments:\n");
	    for (i = 0; i < ac; i++)
		fprintf(stderr, " %s", av[i]);
	    goto exit;
	    /*@notreached@*/ break;
	case 'i':
	case 'u':
	case 'r':
	    for (i = 0; i < ac; i++) {
		char * t = rpmExpand("  ", av[i], NULL); *t = lcmd;
		(void) argvAdd(&sm->av, t);
		t = _free(t);
	    }
	    break;
	}
    }
    
    if (F_ISSET(sm, REBUILD) || F_ISSET(sm, RELOAD)) {
	if (sm->av && *sm->av) {
	    fprintf(stderr, "build or reload should not be used with other commands\n");
	    goto exit;
	}
    } else {
	if (!(sm->av && *sm->av)) {
	    fprintf(stderr, "At least one mode must be specified.\n");
	    goto exit;
	}
    }

    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) signal(SIGTERM, SIG_IGN);

    rc = rpmsmRun(sm, sm->av, &result);
    if (result)
	fprintf((rc < 0 ? stderr : stdout), "%s\n", result);

exit:

    if (optCon)
	optCon = rpmioFini(optCon);

    return (rc < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}