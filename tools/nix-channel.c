#include "system.h"

#include <rpmiotypes.h>
#include <rpmio.h>
#include <rpmlog.h>
#include <rpmmacro.h>
#include <argv.h>
#include <ugid.h>
#include <poptIO.h>

#include "debug.h"

static int _debug = -1;


#define _KFB(n) (1U << (n))
#define _DFB(n) (_KFB(n) | 0x40000000)

#define F_ISSET(_nix, _FLAG) ((_nix)->flags & ((RPMNIX_FLAGS_##_FLAG) & ~0x40000000))

/**
 * Bit field enum for rpmdigest CLI options.
 */
enum nixFlags_e {
    RPMNIX_FLAGS_NONE		= 0,
    RPMNIX_FLAGS_ADDDRVLINK	= _DFB(0),	/*    --add-drv-link */
    RPMNIX_FLAGS_NOOUTLINK	= _DFB(1),	/* -o,--no-out-link */
    RPMNIX_FLAGS_DRYRUN		= _DFB(2),	/*    --dry-run */

    RPMNIX_FLAGS_EVALONLY	= _DFB(16),	/*    --eval-only */
    RPMNIX_FLAGS_PARSEONLY	= _DFB(17),	/*    --parse-only */
    RPMNIX_FLAGS_ADDROOT	= _DFB(18),	/*    --add-root */
    RPMNIX_FLAGS_XML		= _DFB(19),	/*    --xml */
    RPMNIX_FLAGS_STRICT		= _DFB(20),	/*    --strict */
    RPMNIX_FLAGS_SHOWTRACE	= _DFB(21),	/*    --show-trace */

    RPMNIX_FLAGS_SKIPWRONGSTORE	= _DFB(24)	/*    --skip-wrong-store */
};

/**
 */
typedef struct rpmnix_s * rpmnix;

/**
 */
struct rpmnix_s {
    enum nixFlags_e flags;	/*!< rpmnix control bits. */

    const char * outLink;
    const char * drvLink;

    const char ** instArgs;
    const char ** buildArgs;
    const char ** exprs;

    const char * attr;

    int op;
    const char * url;

};

/**
 */
static struct rpmnix_s _nix = {
	.flags = RPMNIX_FLAGS_NOOUTLINK
};

static const char * nixDefExpr;
static const char * channelsList;
static const char ** channels;

static const char * binDir	= "/usr/bin";
static const char * homeDir	= "~";

static const char * stateDir	= "/nix/var/nix";
static const char * rootsDir	= "/nix/var/nix/gcroots";

static const char * channelCache;

#define	DBG(_l)	if (_debug) fprintf _l
/*==============================================================*/

static char * _freeCmd(const char * cmd)
{
DBG((stderr, "\t%s\n", cmd));
    cmd = _free(cmd);
    return NULL;
}

/* Reads the list of channels from the file $channelsList. */
static void readChannels(rpmnix nix)
	/*@*/
{
    FD_t fd;
    struct stat sb;
    int xx;

DBG((stderr, "--> %s(%p)\n", __FUNCTION__, nix));

#ifdef	REFERENCE
/*
    return if (!-f $channelsList);
*/
#endif
    if (channelsList == NULL || Stat(channelsList, &sb) < 0)
	return;
#ifdef	REFERENCE
/*
    open CHANNELS, "<$channelsList" or die "cannot open `$channelsList': $!";
    while (<CHANNELS>) {
        chomp;
        next if /^\s*\#/;
        push @channels, $_;
    }
    close CHANNELS;
*/
#endif
    fd = Fopen(channelsList, "r.fpio");
    if (fd == NULL || Ferror(fd)) {
	fprintf(stderr, "Fopen(%s, \"r\") failed.\n", channelsList);
	if (fd) xx = Fclose(fd);
	exit(1);
    }
    /* XXX skip comments todo++ */
    channels = argvFree(channels);
    xx = argvFgets(&channels, fd);
    xx = Fclose(fd);
}

/* Writes the list of channels to the file $channelsList */
static void writeChannels(rpmnix nix)
	/*@*/
{
    FD_t fd;
    int ac = argvCount(channels);
    ssize_t nw;
    int xx;
    int i;

DBG((stderr, "--> %s(%p)\n", __FUNCTION__, nix));
    if (Access(channelsList, W_OK)) {
	fprintf(stderr, "file %s is not writable.\n", channelsList);
	return;
    }
    fd = Fopen(channelsList, "w");
    if (fd == NULL || Ferror(fd)) {
	fprintf(stderr, "Fopen(%s, \"w\") failed.\n", channelsList);
	if (fd) xx = Fclose(fd);
	exit(1);
    }
    for (i = 0; i < ac; i++) {
	const char * url = channels[i];
	nw = Fwrite(url, 1, strlen(url), fd);
	nw = Fwrite("\n", 1, sizeof("\n")-1, fd);
    }
    xx = Fclose(fd);
}

/* Adds a channel to the file $channelsList */
static void addChannel(rpmnix nix, const char * url)
	/*@*/
{
    int ac;
    int xx;
    int i;

DBG((stderr, "--> %s(%p, \"%s\")\n", __FUNCTION__, nix, url));
    readChannels(nix);
    ac = argvCount(channels);
    for (i = 0; i < ac; i++) {
	if (!strcmp(channels[i], url))
	    return;
    }
    xx = argvAdd(&channels, url);
    writeChannels(nix);
}

/* Remove a channel from the file $channelsList */
static void removeChannel(rpmnix nix, const char * url)
	/*@*/
{
    const char ** nchannels = NULL;
    int ac;
    int xx;
    int i;

DBG((stderr, "--> %s(%p, \"%s\")\n", __FUNCTION__, nix, url));
    readChannels(nix);
    ac = argvCount(channels);
    for (i = 0; i < ac; i++) {
	if (!strcmp(channels[i], url))
	    continue;
	xx = argvAdd(&nchannels, channels[i]);
    }
    channels = argvFree(channels);
    channels = nchannels;
    writeChannels(nix);
}

/*
 * Fetch Nix expressions and pull cache manifests from the subscribed
 * channels.
 */
static void updateChannels(rpmnix nix)
	/*@*/
{
    uid_t uid = getuid();
    const char * userName = uidToUname(uid);
    const char * rootFile;
#ifdef	UNUSED
    const char * rval;
#endif
    const char * cmd;
    const char * dn;
    const char * fn;
    int xx;

DBG((stderr, "--> %s(%p)\n", __FUNCTION__, nix));

    readChannels(nix);

    /* Create the manifests directory if it doesn't exist. */
    dn = rpmGetPath(stateDir, "/manifests", NULL);
    xx = rpmioMkpath(dn, (mode_t)0755, (uid_t)-1, (gid_t)-1);
    dn = _free(dn);

    /*
     * Do we have write permission to the manifests directory?  If not,
     * then just skip pulling the manifest and just download the Nix
     * expressions.  If the user is a non-privileged user in a
     * multi-user Nix installation, he at least gets installation from
     * source.
     */
    dn = rpmGetPath(stateDir, "/manifests", NULL);
    if (!Access(dn, W_OK)) {
	int ac = argvCount(channels);
	int i;

	/* Pull cache manifests. */
#ifdef	REFERENCE
/*
        foreach my $url (@channels) {
            #print "pulling cache manifest from `$url'\n";
            system("/usr/bin/nix-pull", "--skip-wrong-store", "$url/MANIFEST") == 0
                or die "cannot pull cache manifest from `$url'";
        }

*/
#endif
	for (i = 0; i < ac; i++) {
	    const char * url = channels[i];
            cmd = rpmExpand(binDir, "/nix-pull --skip-wrong-store ", url, "/MANIFEST",
			"; echo $?", NULL);
fprintf(stderr, "<-- cmd: %s\n", cmd);
#ifdef	NOTYET
	    rval = rpmExpand("%(", cmd, ")", NULL);
	    if (strcmp(rval, "0")) {
		fprintf(stderr, "cannot pull cache manifest from `%s'\n", url);
		exit(1);
	    }
	    rval = _free(rval);
#endif
	    cmd = _freeCmd(cmd);
	}
    }
    dn = _free(dn);

    /*
     * Create a Nix expression that fetches and unpacks the channel Nix
     * expressions.
     */
#ifdef	REFERENCE
/*
    my $inputs = "[";
    foreach my $url (@channels) {
        $url =~ /\/([^\/]+)\/?$/;
        my $channelName = $1;
        $channelName = "unnamed" unless defined $channelName;

        my $fullURL = "$url/nixexprs.tar.bz2";
        print "downloading Nix expressions from `$fullURL'...\n";
        $ENV{"PRINT_PATH"} = 1;
        $ENV{"QUIET"} = 1;
        my ($hash, $path) = `/usr/bin/nix-prefetch-url '$fullURL'`;
        die "cannot fetch `$fullURL'" if $? != 0;
        chomp $path;
        $inputs .= '"' . $channelName . '"' . " " . $path . " ";
    }
    $inputs .= "]";
*/
#endif

    /* Figure out a name for the GC root. */
    rootFile = rpmGetPath(rootsDir, "/per-user/", userName, "/channels", NULL);
    
    /* Build the Nix expression. */
    fprintf(stdout, "unpacking channel Nix expressions...\n");
#ifdef	REFERENCE
/*
    my $outPath = `\\
        /usr/bin/nix-build --out-link '$rootFile' --drv-link '$rootFile'.tmp \\
        /usr/share/nix/corepkgs/channels/unpack.nix \\
        --argstr system i686-linux --arg inputs '$inputs'`
        or die "cannot unpack the channels";
    chomp $outPath;
*/
#endif

#ifdef	REFERENCE
/*
    unlink "$rootFile.tmp";
*/
#endif
    fn = rpmGetPath(rootFile, ".tmp", NULL);
    xx = Unlink(fn);
    fn = _free(fn);

    /* Make the channels appear in nix-env. */
#ifdef	REFERENCE
/*
    unlink $nixDefExpr if -l $nixDefExpr; # old-skool ~/.nix-defexpr
    mkdir $nixDefExpr or die "cannot create directory `$nixDefExpr'" if !-e $nixDefExpr;
    my $channelLink = "$nixDefExpr/channels";
    unlink $channelLink; # !!! not atomic
    symlink($outPath, $channelLink) or die "cannot symlink `$channelLink' to `$outPath'";
*/
#endif

    rootFile = _free(rootFile);
}

/*==============================================================*/

#ifdef	UNUSED
static int verbose = 0;
#endif

enum {
    NIX_CHANNEL_ADD = 1,
    NIX_CHANNEL_REMOVE,
    NIX_CHANNEL_LIST,
    NIX_CHANNEL_UPDATE,
};

static void nixInstantiateArgCallback(poptContext con,
                /*@unused@*/ enum poptCallbackReason reason,
                const struct poptOption * opt, const char * arg,
                /*@unused@*/ void * data)
	/*@*/
{
    rpmnix nix = &_nix;

    /* XXX avoid accidental collisions with POPT_BIT_SET for flags */
    if (opt->arg == NULL)
    switch (opt->val) {
    case NIX_CHANNEL_ADD:
    case NIX_CHANNEL_REMOVE:
	nix->url = xstrdup(arg);
	/*@fallthrough@*/
    case NIX_CHANNEL_LIST:
    case NIX_CHANNEL_UPDATE:
	nix->op = opt->val;
	break;
    default:
	fprintf(stderr, _("%s: Unknown callback(0x%x)\n"), __FUNCTION__, (unsigned) opt->val);
	poptPrintUsage(con, stderr, 0);
	/*@-exitarg@*/ exit(2); /*@=exitarg@*/
	/*@notreached@*/ break;
    }
}

static struct poptOption nixInstantiateOptions[] = {
/*@-type@*/ /* FIX: cast? */
 { NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_INC_DATA | POPT_CBFLAG_CONTINUE,
	nixInstantiateArgCallback, 0, NULL, NULL },
/*@=type@*/

 { "add", '\0', POPT_ARG_STRING,		0, NIX_CHANNEL_ADD,
        N_("subscribe to a Nix channel"), N_("URL") },
 { "remove", '\0', POPT_ARG_STRING,		0, NIX_CHANNEL_REMOVE,
        N_("unsubscribe from a Nix channel"), N_("URL") },
 { "list", '\0', POPT_ARG_NONE,		0, NIX_CHANNEL_LIST,
        N_("list subscribed channels"), NULL },
 { "update", '\0', POPT_ARG_NONE,		0, NIX_CHANNEL_UPDATE,
        N_("download latest Nix expressions"), NULL },

#ifndef	XXXNOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_("Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOHELP

#ifdef	DYING
  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
	N_("\
Usage:\n\
  nix-channel --add URL\n\
  nix-channel --remove URL\n\
  nix-channel --list\n\
  nix-channel --update\n\
"), NULL },
#endif

  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    rpmnix nix = &_nix;
    poptContext optCon = rpmioInit(argc, argv, nixInstantiateOptions);
    ARGV_t av = poptGetArgs(optCon);
    int ac = argvCount(av);
    const char * s;
    int ec = 1;		/* assume failure */
    int xx;

    if ((s = getenv("NIX_BIN_DIR"))) binDir = s;
    if ((s = getenv("NIX_STATE_DIR"))) stateDir = s;

    /* Turn on caching in nix-prefetch-url. */
#ifdef	REFERENCE
/*
my $channelCache = "$stateDir/channel-cache";
mkdir $channelCache, 0755 unless -e $channelCache;
$ENV{'NIX_DOWNLOAD_CACHE'} = $channelCache if -W $channelCache;
*/
#endif
    channelCache = rpmGetPath(stateDir, "/channel-cache", NULL);
    xx = rpmioMkpath(channelCache, 0755, (uid_t)-1, (gid_t)-1);
    if (!Access(channelCache, W_OK))
	xx = setenv("NIX_DOWNLOAD_CACHE", channelCache, 0);

    /* Figure out the name of the `.nix-channels' file to use. */
    if ((s = getenv("HOME"))) homeDir = s;
    channelsList = rpmGetPath(homeDir, "/.nix-channels", NULL);
    nixDefExpr = rpmGetPath(homeDir, "/.nix-defexpr", NULL);

    if (nix->op == 0 || ac != 0) {
	poptPrintUsage(optCon, stderr, 0);
	goto exit;
    }

    switch (nix->op) {
    case NIX_CHANNEL_ADD:
assert(nix->url != NULL);	/* XXX proper exit */
	addChannel(nix, nix->url);
	break;
    case NIX_CHANNEL_REMOVE:
assert(nix->url != NULL);	/* XXX proper exit */
	removeChannel(nix, nix->url);
	break;
    case NIX_CHANNEL_LIST:
	readChannels(nix);
	argvPrint(channelsList, channels, NULL);
	break;
    case NIX_CHANNEL_UPDATE:
	updateChannels(nix);
	break;
    }

    ec = 0;	/* XXX success */

exit:

    nix->url = _free(nix->url);

    channels = argvFree(channels);
    channelsList = _free(channelsList);
    nixDefExpr = _free(nixDefExpr);

    optCon = rpmioFini(optCon);

    return ec;
}