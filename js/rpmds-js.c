/** \ingroup js_c
 * \file js/rpmds-js.c
 */

#include "system.h"

#include "rpmds-js.h"
#include "rpmhdr-js.h"
#include "rpmjs-debug.h"

#include <argv.h>
#include <mire.h>

#include <rpmdb.h>

#include <rpmds.h>

#include "debug.h"

/*@unchecked@*/
static int _debug = 0;

/* --- helpers */

/* --- Object methods */

#ifdef	NOTYET
static JSBool
rpmds_next(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject * obj = JS_THIS_OBJECT(cx, vp);
    rpmds ds = (rpmds)JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    JSBool ok = (rpmdsNext(ds) >= 0) ? JS_TRUE : JS_FALSE;

if (_debug)
fprintf(stderr, "==> %s(%p,%u,%p) ds %p[%d:%d] %s\n", __FUNCTION__, cx, argc, vp, ds, rpmdsIx(ds), rpmdsCount(ds), JS_GetStringBytes(JS_ValueToString(cx, *vp)));

    if (!ok)
	goto exit;

    {   JSString *valstr;
	const char *N = rpmdsN(ds);
	const char *EVR = rpmdsEVR(ds);
	unsigned Flags = rpmdsFlags(ds);

	if ((valstr = JS_NewStringCopyZ(cx, N)) == NULL
	 || !JS_DefineProperty(cx, obj, "N", STRING_TO_JSVAL(valstr),
				NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
 	if ((valstr = JS_NewStringCopyZ(cx, EVR)) == NULL
	 || !JS_DefineProperty(cx, obj, "EVR", STRING_TO_JSVAL(valstr),
				NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
 	if (!JS_DefineProperty(cx, obj, "F", INT_TO_JSVAL(Flags),
				NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
    }

exit:
    return ok;
}
#endif

static JSBool
rpmds_self(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject * obj = JS_THIS_OBJECT(cx, vp);
    rpmds ds = (rpmds) JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);

    ds = rpmdsInit(ds);

    *vp = JS_THIS(cx, vp);

if (_debug)
fprintf(stderr, "==> %s(%p,%u,%p) ds %p[%d:%d] %s\n", __FUNCTION__, cx, argc, vp, ds, rpmdsIx(ds), rpmdsCount(ds), JS_GetStringBytes(JS_ValueToString(cx, *vp)));

    return !JSVAL_IS_NULL(*vp);
}

static JSFunctionSpec rpmds_funcs[] = {
#ifdef	NOTYET
#define	JSPROP_ROPERM	(JSPROP_READONLY | JSPROP_PERMANENT)
    JS_FN(js_iterator_str,	rpmds_self,	0,0,JSPROP_ROPERM),
    JS_FN(js_next_str,		rpmds_next,	0,0,JSPROP_ROPERM),
#endif
    JS_FS_END
};

/* --- Object properties */
enum rpmds_tinyid {
    _DEBUG	= -2,
    _LENGTH	= -3,
    _TYPE	= -4,
    _IX		= -5,
    _BUILDTIME	= -6,
    _COLOR	= -7,
    _NOPROMOTE	= -8,
    _N		= -9,
    _EVR	= -10,
    _F		= -11,
    _DNEVR	= -12,
};

static JSPropertySpec rpmds_props[] = {
    {"debug",	_DEBUG,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"length",	_LENGTH,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"type",	_TYPE,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"ix",	_IX,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"buildtime",_BUILDTIME,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"color",	_COLOR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"nopromote",_NOPROMOTE,	JSPROP_ENUMERATE,	NULL,	NULL},
    {"N",	_N,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"EVR",	_EVR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"F",	_F,		JSPROP_ENUMERATE,	NULL,	NULL},
    {"DNEVR",	_DNEVR,		JSPROP_ENUMERATE,	NULL,	NULL},
    {NULL, 0, 0, NULL, NULL}
};

static JSBool
rpmds_addprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);

_PROP_DEBUG_ENTRY(_debug < 0);

    return JS_TRUE;
}

static JSBool
rpmds_delprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);

_PROP_DEBUG_ENTRY(_debug < 0);

    return JS_TRUE;
}
static JSBool
rpmds_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = ptr;
    jsint tiny = JSVAL_TO_INT(id);
    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    JSBool ok = (ptr ? JS_FALSE : JS_TRUE);

    switch (tiny) {
    case _DEBUG:
	*vp = INT_TO_JSVAL(_debug);
	ok = JS_TRUE;
	break;
    case _LENGTH:
	*vp = INT_TO_JSVAL(rpmdsCount(ds));
	ok = JS_TRUE;
	break;
    case _TYPE:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsType(ds)));
        ok = JS_TRUE;
        break;
    case _IX:
	*vp = INT_TO_JSVAL(rpmdsIx(ds));
        ok = JS_TRUE;
        break;
    case _BUILDTIME:
	*vp = INT_TO_JSVAL(rpmdsBT(ds));
        ok = JS_TRUE;
        break;
    case _COLOR:
	*vp = INT_TO_JSVAL(rpmdsColor(ds));
        ok = JS_TRUE;
        break;
    case _NOPROMOTE:
	*vp = INT_TO_JSVAL(rpmdsNoPromote(ds));
        ok = JS_TRUE;
        break;
    case _N:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsN(ds)));
        ok = JS_TRUE;
        break;
    case _EVR:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsEVR(ds)));
        ok = JS_TRUE;
        break;
    case _F:
	*vp = INT_TO_JSVAL(rpmdsFlags(ds));
        ok = JS_TRUE;
        break;
    case _DNEVR:
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, rpmdsDNEVR(ds)));
        ok = JS_TRUE;
        break;
    default:
	if (tiny >= 0 && tiny < rpmdsCount(ds))
	    ok = JS_TRUE;
	break;
    }

    if (!ok) {
_PROP_DEBUG_EXIT(_debug);
    }

    return ok;
}

static JSBool
rpmds_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = (rpmds)ptr;
    jsint tiny = JSVAL_TO_INT(id);
    /* XXX the class has ptr == NULL, instances have ptr != NULL. */
    JSBool ok = (ptr ? JS_FALSE : JS_TRUE);
    int myint;

    switch (tiny) {
    case _DEBUG:
	if (JS_ValueToInt32(cx, *vp, &_debug))
	    ok = JS_TRUE;
	break;
    case _IX:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	if (myint != rpmdsIx(ds)) {
	    (void) rpmdsSetIx(ds, myint-1);
	    /* XXX flush and recreate N and DNEVR with a rpmdsNext() step */
	    (void) rpmdsNext(ds);
	}
	ok = JS_TRUE;
        break;
    case _BUILDTIME:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmdsSetBT(ds, myint);
	ok = JS_TRUE;
        break;
    case _NOPROMOTE:
	if (!JS_ValueToInt32(cx, *vp, &myint))
	    break;
	(void) rpmdsSetNoPromote(ds, myint);
	ok = JS_TRUE;
        break;
    default:
	break;
    }

    if (!ok) {
_PROP_DEBUG_EXIT(_debug);
    }
    return ok;
}

static JSBool
rpmds_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
	JSObject **objp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = ptr;
    jsint ix;
    JSBool ok = JS_FALSE;

_RESOLVE_DEBUG_ENTRY(_debug < 0);

    if ((flags & JSRESOLVE_ASSIGNING)
     || (ds == NULL)) {	/* don't resolve to parent prototypes objects. */
	*objp = NULL;
	ok = JS_TRUE;
	goto exit;
    }

    if (JSVAL_IS_INT(id) && (ix = JSVAL_TO_INT(id)) == rpmdsIx(ds)) {
	JSObject * arr = JS_NewArrayObject(cx, 3, NULL);
	JSString *valstr;
	const char *N = rpmdsN(ds);
	const char *EVR = rpmdsEVR(ds);
	unsigned Flags = rpmdsFlags(ds);

	if (!JS_DefineElement(cx, obj, ix, OBJECT_TO_JSVAL(arr),
			NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
	if ((valstr = JS_NewStringCopyZ(cx, N)) == NULL
         || !JS_DefineElement(cx, arr, 0, STRING_TO_JSVAL(valstr),
                        NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
 	if ((valstr = JS_NewStringCopyZ(cx, EVR)) == NULL
         || !JS_DefineElement(cx, arr, 1, STRING_TO_JSVAL(valstr),
                        NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
        if (!JS_DefineElement(cx, arr, 2, INT_TO_JSVAL(Flags),
                        NULL, NULL, JSPROP_ENUMERATE))
	    goto exit;
	*objp = obj;
    } else
	*objp = NULL;

    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmds_enumerate(JSContext *cx, JSObject *obj, JSIterateOp op,
		  jsval *statep, jsid *idp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = (rpmds)ptr;
    JSObject *iterobj;
    JSBool ok = JS_FALSE;
    int ix;

_ENUMERATE_DEBUG_ENTRY(_debug < 0);

    switch (op) {
    case JSENUMERATE_INIT:
	*statep = JSVAL_VOID;
	(void) rpmdsInit(ds);
        if (idp)
            *idp = JSVAL_ZERO;
        break;
    case JSENUMERATE_NEXT:
	*statep = JSVAL_VOID;
	if ((ix = rpmdsNext(ds)) >= 0) {
	    JS_ValueToId(cx, INT_TO_JSVAL(ix), idp);
	} else
	    *idp = JSVAL_VOID;
        if (*idp != JSVAL_VOID)
            break;
        /*@fallthrough@*/
    case JSENUMERATE_DESTROY:
	*statep = JSVAL_NULL;
        break;
    }
    ok = JS_TRUE;
exit:
    return ok;
}

static JSBool
rpmds_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);

_CONVERT_DEBUG_ENTRY(_debug);

    return JS_TRUE;
}

/* --- Object ctors/dtors */
static rpmds
rpmds_init(JSContext *cx, JSObject *obj, JSObject *o, int _tagN)
{
    rpmds ds = NULL;
    int xx;

    if (OBJ_IS_RPMHDR(cx, o)) {
	Header h = JS_GetPrivate(cx, o);
	int flags = 0;
	if (_tagN == RPMTAG_NAME) {
	    flags = RPMSENSE_EQUAL;
	    _tagN = RPMTAG_PROVIDENAME;
	    ds = rpmdsThis(h, _tagN, flags);
if (_debug)
fprintf(stderr, "\trpmdsThis(%p, %s(%d), 0x%x) ds %p\n", h, tagName(_tagN), _tagN, flags, ds);
	} else {
	    ds = rpmdsNew(h, _tagN, flags);
if (_debug)
fprintf(stderr, "\trpmdsNew(%p, %s(%d), 0x%x) ds %p\n", h, tagName(_tagN), _tagN, flags, ds);
	}
	if (ds == NULL)
	    return NULL;
    } else
    if (OBJ_IS_STRING(cx, o)) {
	const char * s =
		JS_GetStringBytes(JS_ValueToString(cx, OBJECT_TO_JSVAL(o)));
	if (!strcmp(s, "cpuinfo")) {
	    xx = rpmdsCpuinfo(&ds, NULL);
if (_debug)
fprintf(stderr, "\trpmdsCpuinfo() ret %d ds %p\n", xx, ds);
	} else
	if (!strcmp(s, "rpmlib")) {
	    xx = rpmdsRpmlib(&ds, NULL);
if (_debug)
fprintf(stderr, "\trpmdsRpmlib() ret %d ds %p\n", xx, ds);
	} else
	if (!strcmp(s, "getconf")) {
	    xx = rpmdsGetconf(&ds, NULL);
if (_debug)
fprintf(stderr, "\trpmdsGetconf() ret %d ds %p\n", xx, ds);
	} else
	if (!strcmp(s, "uname")) {
	    xx = rpmdsUname(&ds, NULL);
if (_debug)
fprintf(stderr, "\trpmdsUname() ret %d ds %p\n", xx, ds);
	} else
	{
if (_debug)
fprintf(stderr, "\tstring \"%s\" is unknown. ds %p\n", s, ds);
	    return NULL;
	}
    } else
    if (OBJ_IS_ARRAY(cx, o)) {
	jsuint length = 0;
	jsuint i;
	JSBool ok = JS_GetArrayLength(cx, o, &length);
	const char * N = NULL;
	const char * EVR = NULL;
	uint32_t F = 0;

	if (!ok)
	    return NULL;
	if (length != 3)
	    return NULL;
	for (i = 0; i < length; i++) {
	    jsval v;
	    if (!(ok = JS_GetElement(cx, o, (jsint)i, &v)))
		return NULL;
	    switch (i) {
	    default:
		return NULL;
		/*@notreached@*/ break;
	    case 0:
		N = JS_GetStringBytes(JSVAL_TO_STRING(v));
		break;
	    case 1:
		EVR = JS_GetStringBytes(JSVAL_TO_STRING(v));
		break;
	    case 2:
		F = JSVAL_TO_INT(v);
		break;
	    }
	}
	ds = rpmdsSingle(_tagN, N, EVR, F);
if (_debug)
fprintf(stderr, "\trpmdsSingle(%s(%d), %s, %s, 0x%x) ds %p\n", tagName(_tagN), _tagN, N, EVR, F, ds);
	return NULL;
    } else {
if (_debug)
fprintf(stderr, "\tobject class %p is unknown. ds %p\n", OBJ_GET_CLASS(cx, o), ds);
	return NULL;
    }
    if (!JS_SetPrivate(cx, obj, (void *)ds)) {
	/* XXX error msg */
	(void) rpmdsFree(ds);
	return NULL;
    }
    return ds;
}

static void
rpmds_dtor(JSContext *cx, JSObject *obj)
{
    void * ptr = JS_GetInstancePrivate(cx, obj, &rpmdsClass, NULL);
    rpmds ds = ptr;

if (_debug)
fprintf(stderr, "==> %s(%p,%p) ptr %p\n", __FUNCTION__, cx, obj, ptr);

    (void) rpmdsFree(ds);
}

static JSBool
rpmds_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSBool ok = JS_FALSE;
    JSObject *o = NULL;
    int tagN = RPMTAG_REQUIRENAME;

if (_debug)
fprintf(stderr, "==> %s(%p,%p,%p[%u],%p)%s\n", __FUNCTION__, cx, obj, argv, (unsigned)argc, rval, ((cx->fp->flags & JSFRAME_CONSTRUCTING) ? " constructing" : ""));

    if (!(ok = JS_ConvertArguments(cx, argc, argv, "o/i", &o, &tagN)))
	goto exit;

    if (cx->fp->flags & JSFRAME_CONSTRUCTING) {
	if (rpmds_init(cx, obj, o, tagN) == NULL)
	    goto exit;
    } else {
	if ((obj = JS_NewObject(cx, &rpmdsClass, NULL, NULL)) == NULL)
	    goto exit;
	*rval = OBJECT_TO_JSVAL(obj);
    }
    ok = JS_TRUE;

exit:
    return ok;
}

/* --- Class initialization */
JSClass rpmdsClass = {
    "Ds", JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE | JSCLASS_HAS_PRIVATE,
    rpmds_addprop,   rpmds_delprop, rpmds_getprop, rpmds_setprop,
    (JSEnumerateOp)rpmds_enumerate, (JSResolveOp)rpmds_resolve,
    rpmds_convert,	rpmds_dtor,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
rpmjs_InitDsClass(JSContext *cx, JSObject* obj)
{
    JSObject * o;

if (_debug)
fprintf(stderr, "==> %s(%p,%p)\n", __FUNCTION__, cx, obj);

    o = JS_InitClass(cx, obj, NULL, &rpmdsClass, rpmds_ctor, 1,
		rpmds_props, rpmds_funcs, NULL, NULL);
assert(o != NULL);
    return o;
}

JSObject *
rpmjs_NewDsObject(JSContext *cx, JSObject *o, int _tagN)
{
    JSObject *obj;
    rpmds ds;

    if ((obj = JS_NewObject(cx, &rpmdsClass, NULL, NULL)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    if ((ds = rpmds_init(cx, obj, o, _tagN)) == NULL) {
	/* XXX error msg */
	return NULL;
    }
    return obj;
}