#ifndef H_RPMRC_PY
#define H_RPMRC_PY

/** \ingroup py_c
 * \file python/rpmrc-py.h
 */

/** \name Type: _rpm.rc */
/*@{*/

/** \ingroup py_c
 */
typedef struct rpmrcObject_s rpmrcObject;

/** \ingroup py_c
 */
struct rpmrcObject_s {
#if Py_TPFLAGS_HAVE_ITER	/* XXX backport to python-1.5.2 */
    PyDictObject dict;
#else
    PyObject_HEAD
#endif
    PyObject *md_dict;		/*!< to look like PyModuleObject */
} ;

/*@unchecked@*/
extern PyTypeObject rpmrc_Type;

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup py_c
 */
/*@null@*/
PyObject * rpmrc_AddMacro(PyObject * self, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies rpmGlobalMacroContext, _Py_NoneStruct @*/;

/** \ingroup py_c
 */
/*@null@*/
PyObject * rpmrc_DelMacro(PyObject * self, PyObject * args, PyObject * kwds)
	/*@globals rpmGlobalMacroContext, _Py_NoneStruct @*/
	/*@modifies rpmGlobalMacroContext, _Py_NoneStruct @*/;

#if Py_TPFLAGS_HAVE_ITER	/* XXX backport to python-1.5.2 */
/** \ingroup py_c
 */
/*@null@*/
PyObject * rpmrc_Create(PyObject * self, PyObject * args, PyObject * kwds)
	/*@*/;
#endif

#ifdef __cplusplus      
}
#endif

/*@}*/

#endif