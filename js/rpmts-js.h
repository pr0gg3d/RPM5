#ifndef H_RPMTS_JS
#define H_RPMTS_JS

/**
 * \file js/rpmts-js.h
 */

#include "rpm-js.h"

extern JSClass rpmtsClass;

#ifdef __cplusplus
extern "C" {
#endif

extern JSObject*
rpmjs_InitTsClass(JSContext *cx, JSObject* obj);

#ifdef __cplusplus      
}
#endif

#endif	/* H_RPMTS_JS */