/**
 * $Id$
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Michel Selten, Willian P. Germano, Stephen Swaney,
 * Chris Keith, Chris Want, Ken Hughes, Campbell Barton
 *
 * ***** END GPL LICENSE BLOCK *****
 */
 
/* grr, python redefines */
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#include <Python.h>

#include "bpy.h"
#include "bpy_rna.h"
#include "bpy_util.h"

#include "DNA_space_types.h"
#include "DNA_text_types.h"

#include "MEM_guardedalloc.h"
#include "BLI_path_util.h"
#include "BLI_math_base.h"
#include "BLI_string.h"

#include "BKE_utildefines.h"
#include "BKE_context.h"
#include "BKE_text.h"
#include "BKE_font.h" /* only for utf8towchar */
#include "BKE_main.h"
#include "BKE_global.h" /* only for script checking */

#include "BPY_extern.h"

#include "../generic/bpy_internal_import.h" // our own imports
#include "../generic/py_capi_utils.h"

/* for internal use, when starting and ending python scripts */

/* incase a python script triggers another python call, stop bpy_context_clear from invalidating */
static int py_call_level= 0;
BPy_StructRNA *bpy_context_module= NULL; /* for fast access */

// only for tests
#define TIME_PY_RUN

#ifdef TIME_PY_RUN
#include "PIL_time.h"
static int		bpy_timer_count = 0;
static double	bpy_timer; /* time since python starts */
static double	bpy_timer_run; /* time for each python script run */
static double	bpy_timer_run_tot; /* accumulate python runs */
#endif

void bpy_context_set(bContext *C, PyGILState_STATE *gilstate)
{
	py_call_level++;

	if(gilstate)
		*gilstate = PyGILState_Ensure();

	if(py_call_level==1) {

		if(C) { // XXX - should always be true.
			BPy_SetContext(C);
			bpy_import_main_set(CTX_data_main(C));
		}
		else {
			fprintf(stderr, "ERROR: Python context called with a NULL Context. this should not happen!\n");
		}

		BPY_update_modules(); /* can give really bad results if this isnt here */

#ifdef TIME_PY_RUN
		if(bpy_timer_count==0) {
			/* record time from the beginning */
			bpy_timer= PIL_check_seconds_timer();
			bpy_timer_run = bpy_timer_run_tot = 0.0;
		}
		bpy_timer_run= PIL_check_seconds_timer();


		bpy_timer_count++;
#endif
	}
}

/* context should be used but not now because it causes some bugs */
void bpy_context_clear(bContext *UNUSED(C), PyGILState_STATE *gilstate)
{
	py_call_level--;

	if(gilstate)
		PyGILState_Release(*gilstate);

	if(py_call_level < 0) {
		fprintf(stderr, "ERROR: Python context internal state bug. this should not happen!\n");
	}
	else if(py_call_level==0) {
		// XXX - Calling classes currently wont store the context :\, cant set NULL because of this. but this is very flakey still.
		//BPy_SetContext(NULL);
		//bpy_import_main_set(NULL);

#ifdef TIME_PY_RUN
		bpy_timer_run_tot += PIL_check_seconds_timer() - bpy_timer_run;
		bpy_timer_count++;
#endif

	}
}

void BPY_free_compiled_text( struct Text *text )
{
	if( text->compiled ) {
		Py_DECREF( ( PyObject * ) text->compiled );
		text->compiled = NULL;
	}
}

void BPY_update_modules( void )
{
#if 0 // slow, this runs all the time poll, draw etc 100's of time a sec.
	PyObject *mod= PyImport_ImportModuleLevel("bpy", NULL, NULL, NULL, 0);
	PyModule_AddObject( mod, "data", BPY_rna_module() );
	PyModule_AddObject( mod, "types", BPY_rna_types() ); // atm this does not need updating
#endif

	/* refreshes the main struct */
	BPY_update_rna_module();
	bpy_context_module->ptr.data= (void *)BPy_GetContext();
}

/* must be called before Py_Initialize */
void BPY_start_python_path(void)
{
	char *py_path_bundle= BLI_get_folder(BLENDER_PYTHON, NULL);

	if(py_path_bundle==NULL)
		return;

	/* set the environment path */
	printf("found bundled python: %s\n", py_path_bundle);

#ifdef __APPLE__
	/* OSX allow file/directory names to contain : character (represented as / in the Finder)
	 but current Python lib (release 3.1.1) doesn't handle these correctly */
	if(strchr(py_path_bundle, ':'))
		printf("Warning : Blender application is located in a path containing : or / chars\
			   \nThis may make python import function fail\n");
#endif
	
#ifdef _WIN32
	/* cmake/MSVC debug build crashes without this, why only
	   in this case is unknown.. */
	{
		BLI_setenv("PYTHONPATH", py_path_bundle);	
	}
#endif

	{
		static wchar_t py_path_bundle_wchar[FILE_MAX];

		/* cant use this, on linux gives bug: #23018, TODO: try LANG="en_US.UTF-8" /usr/bin/blender, suggested 22008 */
		/* mbstowcs(py_path_bundle_wchar, py_path_bundle, FILE_MAXDIR); */

		utf8towchar(py_path_bundle_wchar, py_path_bundle);

		Py_SetPythonHome(py_path_bundle_wchar);
		// printf("found python (wchar_t) '%ls'\n", py_path_bundle_wchar);
	}
}



void BPY_set_context(bContext *C)
{
	BPy_SetContext(C);
}

/* init-tab */
extern PyObject *BPyInit_noise(void);
extern PyObject *BPyInit_mathutils(void);
extern PyObject *BPyInit_bgl(void);
extern PyObject *BPyInit_blf(void);
extern PyObject *AUD_initPython(void);

static struct _inittab bpy_internal_modules[]= {
	{"noise", BPyInit_noise},
	{"mathutils", BPyInit_mathutils},
	{"bgl", BPyInit_bgl},
	{"blf", BPyInit_blf},
	{"aud", AUD_initPython},
	{NULL, NULL}
};

/* call BPY_set_context first */
void BPY_start_python( int argc, char **argv )
{
	PyThreadState *py_tstate = NULL;
	
	/* not essential but nice to set our name */
	static wchar_t bprogname_wchar[FILE_MAXDIR+FILE_MAXFILE]; /* python holds a reference */
	utf8towchar(bprogname_wchar, bprogname);
	Py_SetProgramName(bprogname_wchar);

	/* builtin modules */
	PyImport_ExtendInittab(bpy_internal_modules);

	BPY_start_python_path(); /* allow to use our own included python */

	Py_Initialize(  );
	
	// PySys_SetArgv( argc, argv); // broken in py3, not a huge deal
	/* sigh, why do python guys not have a char** version anymore? :( */
	{
		int i;
		PyObject *py_argv= PyList_New(argc);
		for (i=0; i<argc; i++)
			PyList_SET_ITEM(py_argv, i, PyC_UnicodeFromByte(argv[i])); /* should fix bug #20021 - utf path name problems, by replacing PyUnicode_FromString */

		PySys_SetObject("argv", py_argv);
		Py_DECREF(py_argv);
	}
	
	/* Initialize thread support (also acquires lock) */
	PyEval_InitThreads();
	
	
	/* bpy.* and lets us import it */
	BPy_init_modules();

	{ /* our own import and reload functions */
		PyObject *item;
		//PyObject *m = PyImport_AddModule("__builtin__");
		//PyObject *d = PyModule_GetDict(m);
		PyObject *d = PyEval_GetBuiltins(  );
		PyDict_SetItemString(d, "reload",		item=PyCFunction_New(&bpy_reload_meth, NULL));	Py_DECREF(item);
		PyDict_SetItemString(d, "__import__",	item=PyCFunction_New(&bpy_import_meth, NULL));	Py_DECREF(item);
	}
	
	pyrna_alloc_types();

	py_tstate = PyGILState_GetThisThreadState();
	PyEval_ReleaseThread(py_tstate);
}

void BPY_end_python( void )
{
	// fprintf(stderr, "Ending Python!\n");

	PyGILState_Ensure(); /* finalizing, no need to grab the state */
	
	// free other python data.
	pyrna_free_types();

	/* clear all python data from structs */
	
	Py_Finalize(  );
	
#ifdef TIME_PY_RUN
	// measure time since py started
	bpy_timer = PIL_check_seconds_timer() - bpy_timer;

	printf("*bpy stats* - ");
	printf("tot exec: %d,  ", bpy_timer_count);
	printf("tot run: %.4fsec,  ", bpy_timer_run_tot);
	if(bpy_timer_count>0)
		printf("average run: %.6fsec,  ", (bpy_timer_run_tot/bpy_timer_count));

	if(bpy_timer>0.0)
		printf("tot usage %.4f%%", (bpy_timer_run_tot/bpy_timer)*100.0);

	printf("\n");

	// fprintf(stderr, "Ending Python Done!\n");

#endif

}

/* Can run a file or text block */
int BPY_run_python_script( bContext *C, const char *fn, struct Text *text, struct ReportList *reports)
{
	PyObject *py_dict, *py_result= NULL;
	PyGILState_STATE gilstate;
	
	if (fn==NULL && text==NULL) {
		return 0;
	}

	bpy_context_set(C, &gilstate);

	if (text) {
		char fn_dummy[FILE_MAXDIR];
		bpy_text_filename_get(fn_dummy, text);
		
		if( !text->compiled ) {	/* if it wasn't already compiled, do it now */
			char *buf = txt_to_buf( text );

			text->compiled =
				Py_CompileString( buf, fn_dummy, Py_file_input );

			MEM_freeN( buf );

			if( PyErr_Occurred(  ) ) {
				BPY_free_compiled_text( text );
			}
		}

		if(text->compiled) {
			py_dict = PyC_DefaultNameSpace(fn_dummy);
			py_result =  PyEval_EvalCode(text->compiled, py_dict, py_dict);
		}
		
	}
	else {
		FILE *fp= fopen(fn, "r");

		if(fp) {
			py_dict = PyC_DefaultNameSpace(fn);

#ifdef _WIN32
			/* Previously we used PyRun_File to run directly the code on a FILE 
			 * object, but as written in the Python/C API Ref Manual, chapter 2,
			 * 'FILE structs for different C libraries can be different and 
			 * incompatible'.
			 * So now we load the script file data to a buffer */
			{
				char *pystring;

				fclose(fp);

				pystring= MEM_mallocN(strlen(fn) + 32, "pystring");
				pystring[0]= '\0';
				sprintf(pystring, "exec(open(r'%s').read())", fn);
				py_result = PyRun_String( pystring, Py_file_input, py_dict, py_dict );
				MEM_freeN(pystring);
			}
#else
			py_result = PyRun_File(fp, fn, Py_file_input, py_dict, py_dict);
			fclose(fp);
#endif
		}
		else {
			PyErr_Format(PyExc_SystemError, "Python file \"%s\" could not be opened: %s", fn, strerror(errno));
			py_result= NULL;
		}
	}
	
	if (!py_result) {
		BPy_errors_to_report(reports);
	} else {
		Py_DECREF( py_result );
	}
	
	PyDict_SetItemString(PyThreadState_GET()->interp->modules, "__main__", Py_None);
	
	bpy_context_clear(C, &gilstate);

	return py_result ? 1:0;
}


/* TODO - move into bpy_space.c ? */
/* GUI interface routines */

/* Copied from Draw.c */
static void exit_pydraw( SpaceScript * sc, short err )
{
	Script *script = NULL;

	if( !sc || !sc->script )
		return;

	script = sc->script;

	if( err ) {
		BPy_errors_to_report(NULL); // TODO, reports
		script->flags = 0;	/* mark script struct for deletion */
		SCRIPT_SET_NULL(script);
		script->scriptname[0] = '\0';
		script->scriptarg[0] = '\0';
// XXX 2.5		error_pyscript();
// XXX 2.5		scrarea_queue_redraw( sc->area );
	}

#if 0 // XXX 2.5
	BPy_Set_DrawButtonsList(sc->but_refs);
	BPy_Free_DrawButtonsList(); /*clear all temp button references*/
#endif

	sc->but_refs = NULL;
	
	Py_XDECREF( ( PyObject * ) script->py_draw );
	Py_XDECREF( ( PyObject * ) script->py_event );
	Py_XDECREF( ( PyObject * ) script->py_button );

	script->py_draw = script->py_event = script->py_button = NULL;
}

static int bpy_run_script_init(bContext *C, SpaceScript * sc)
{
	if (sc->script==NULL) 
		return 0;
	
	if (sc->script->py_draw==NULL && sc->script->scriptname[0] != '\0')
		BPY_run_python_script(C, sc->script->scriptname, NULL, NULL);
		
	if (sc->script->py_draw==NULL)
		return 0;
	
	return 1;
}

int BPY_run_script_space_draw(const struct bContext *C, SpaceScript * sc)
{
	if (bpy_run_script_init( (bContext *)C, sc)) {
		PyGILState_STATE gilstate = PyGILState_Ensure();
		PyObject *result = PyObject_CallObject( sc->script->py_draw, NULL );
		
		if (result==NULL)
			exit_pydraw(sc, 1);
			
		PyGILState_Release(gilstate);
	}
	return 1;
}

// XXX - not used yet, listeners dont get a context
int BPY_run_script_space_listener(bContext *C, SpaceScript * sc)
{
	if (bpy_run_script_init(C, sc)) {
		PyGILState_STATE gilstate = PyGILState_Ensure();
		
		PyObject *result = PyObject_CallObject( sc->script->py_draw, NULL );
		
		if (result==NULL)
			exit_pydraw(sc, 1);
			
		PyGILState_Release(gilstate);
	}
	return 1;
}

void BPY_DECREF(void *pyob_ptr)
{
	PyGILState_STATE gilstate = PyGILState_Ensure();
	Py_DECREF((PyObject *)pyob_ptr);
	PyGILState_Release(gilstate);
}

#if 0
/* called from the the scripts window, assume context is ok */
int BPY_run_python_script_space(const char *modulename, const char *func)
{
	PyObject *py_dict, *py_result= NULL;
	char pystring[512];
	PyGILState_STATE gilstate;
	
	/* for calling the module function */
	PyObject *py_func, 
	
	gilstate = PyGILState_Ensure();
	
	py_dict = PyC_DefaultNameSpace("<dummy>");
	
	PyObject *module = PyImport_ImportModule(scpt->script.filename);
	if (module==NULL) {
		PyErr_SetFormat(PyExc_SystemError, "could not import '%s'", scpt->script.filename);
	}
	else {
		py_func = PyObject_GetAttrString(modulename, func);
		if (py_func==NULL) {
			PyErr_SetFormat(PyExc_SystemError, "module has no function '%s.%s'\n", scpt->script.filename, func);
		}
		else {
			Py_DECREF(py_func);
			if (!PyCallable_Check(py_func)) {
				PyErr_SetFormat(PyExc_SystemError, "module item is not callable '%s.%s'\n", scpt->script.filename, func);
			}
			else {
				py_result= PyObject_CallObject(py_func, NULL); // XXX will need args eventually
			}
		}
	}
	
	if (!py_result) {
		BPy_errors_to_report(NULL); // TODO - reports
	} else
		Py_DECREF( py_result );
	
	Py_XDECREF(module);
	
	PyDict_SetItemString(PyThreadState_GET()->interp->modules, "__main__", Py_None);
	
	PyGILState_Release(gilstate);
	return 1;
}
#endif


int BPY_eval_button(bContext *C, const char *expr, double *value)
{
	PyGILState_STATE gilstate;
	PyObject *py_dict, *mod, *retval;
	int error_ret = 0;
	
	if (!value || !expr) return -1;

	if(expr[0]=='\0') {
		*value= 0.0;
		return error_ret;
	}

	bpy_context_set(C, &gilstate);
	
	py_dict= PyC_DefaultNameSpace("<blender button>");

	mod = PyImport_ImportModule("math");
	if (mod) {
		PyDict_Merge(py_dict, PyModule_GetDict(mod), 0); /* 0 - dont overwrite existing values */
		Py_DECREF(mod);
	}
	else { /* highly unlikely but possibly */
		PyErr_Print();
		PyErr_Clear();
	}
	
	retval = PyRun_String(expr, Py_eval_input, py_dict, py_dict);
	
	if (retval == NULL) {
		error_ret= -1;
	}
	else {
		double val;

		if(PyTuple_Check(retval)) {
			/* Users my have typed in 10km, 2m
			 * add up all values */
			int i;
			val= 0.0;

			for(i=0; i<PyTuple_GET_SIZE(retval); i++) {
				val+= PyFloat_AsDouble(PyTuple_GET_ITEM(retval, i));
			}
		}
		else {
			val = PyFloat_AsDouble(retval);
		}
		Py_DECREF(retval);
		
		if(val==-1 && PyErr_Occurred()) {
			error_ret= -1;
		}
		else if (!finite(val)) {
			*value= 0.0;
		}
		else {
			*value= val;
		}
	}
	
	if(error_ret) {
		BPy_errors_to_report(CTX_wm_reports(C));
	}

	PyDict_SetItemString(PyThreadState_GET()->interp->modules, "__main__", Py_None);
	
	bpy_context_clear(C, &gilstate);
	
	return error_ret;
}

int BPY_eval_string(bContext *C, const char *expr)
{
	PyGILState_STATE gilstate;
	PyObject *py_dict, *retval;
	int error_ret = 0;

	if (!expr) return -1;

	if(expr[0]=='\0') {
		return error_ret;
	}

	bpy_context_set(C, &gilstate);

	py_dict= PyC_DefaultNameSpace("<blender string>");

	retval = PyRun_String(expr, Py_eval_input, py_dict, py_dict);

	if (retval == NULL) {
		error_ret= -1;

		BPy_errors_to_report(CTX_wm_reports(C));
	}
	else {
		Py_DECREF(retval);
	}

	PyDict_SetItemString(PyThreadState_GET()->interp->modules, "__main__", Py_None);
	
	bpy_context_clear(C, &gilstate);
	
	return error_ret;
}


void BPY_load_user_modules(bContext *C)
{
	PyGILState_STATE gilstate;
	Main *bmain= CTX_data_main(C);
	Text *text;

	/* can happen on file load */
	if(bmain==NULL)
		return;

	bpy_context_set(C, &gilstate);

	for(text=CTX_data_main(C)->text.first; text; text= text->id.next) {
		if(text->flags & TXT_ISSCRIPT && BLI_testextensie(text->id.name+2, ".py")) {
			if(!(G.f & G_SCRIPT_AUTOEXEC)) {
				printf("scripts disabled for \"%s\", skipping '%s'\n", bmain->name, text->id.name+2);
			}
			else {
				PyObject *module= bpy_text_import(text);

				if (module==NULL) {
					PyErr_Print();
					PyErr_Clear();
				}
				else {
					Py_DECREF(module);
				}
			}
		}
	}
	bpy_context_clear(C, &gilstate);
}

int BPY_context_get(bContext *C, const char *member, bContextDataResult *result)
{
	PyObject *pyctx= (PyObject *)CTX_py_dict_get(C);
	PyObject *item= PyDict_GetItemString(pyctx, member);
	PointerRNA *ptr= NULL;
	int done= 0;

	if(item==NULL) {
		/* pass */
	}
	else if(item==Py_None) {
		/* pass */
	}
	else if(BPy_StructRNA_Check(item)) {
		ptr= &(((BPy_StructRNA *)item)->ptr);

		//result->ptr= ((BPy_StructRNA *)item)->ptr;
		CTX_data_pointer_set(result, ptr->id.data, ptr->type, ptr->data);
		done= 1;
	}
	else if (PySequence_Check(item)) {
		PyObject *seq_fast= PySequence_Fast(item, "bpy_context_get sequence conversion");
		if (seq_fast==NULL) {
			PyErr_Print();
			PyErr_Clear();
		}
		else {
			int len= PySequence_Fast_GET_SIZE(seq_fast);
			int i;
			for(i = 0; i < len; i++) {
				PyObject *list_item= PySequence_Fast_GET_ITEM(seq_fast, i);

				if(BPy_StructRNA_Check(list_item)) {
					/*
					CollectionPointerLink *link= MEM_callocN(sizeof(CollectionPointerLink), "bpy_context_get");
					link->ptr= ((BPy_StructRNA *)item)->ptr;
					BLI_addtail(&result->list, link);
					*/
					ptr= &(((BPy_StructRNA *)list_item)->ptr);
					CTX_data_list_add(result, ptr->id.data, ptr->type, ptr->data);
				}
				else {
					printf("List item not a valid type\n");
				}

			}
			Py_DECREF(seq_fast);

			done= 1;
		}
	}

	if(done==0) {
		if (item)	printf("Context '%s' not a valid type\n", member);
		else		printf("Context '%s' not found\n", member);
	}
	else {
		printf("Context '%s' found\n", member);
	}

	return done;
}

