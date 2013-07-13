/*
 * Fett
 * A tool for building jails with clone(2) and overlayfs
 *
 * Copyright 2013 Uber Technologies, Inc.
 * MIT License
 *
 * This C extension provides access to several useful syscalls
 * that are not available via the Python standard library.
 */
#include <Python.h>
#include <sched.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mount.h>


struct clone_context {
	PyObject *func;
	PyObject *args;
};

int fett_clone_callback(void *ctx) {
	struct clone_context *context = (struct clone_context *)ctx;

	PyObject_CallObject(context->func, context->args);

	Py_DECREF(context->func);
	Py_DECREF(context->args);
	free(context);

	return 0;
}

// child_pid = fett.clone(callback, args, flags)
static PyObject *fett_clone(PyObject *self, PyObject *args) {
	struct clone_context *ctx;
	void **child_stack;
	int child_stack_size;
	int flags;
	int pid;

	ctx = (struct clone_context *)malloc(sizeof(struct clone_context));
	if(ctx == NULL) {
		PyErr_SetString(PyExc_MemoryError, "Unable to allocate memory for clone context");
		return NULL;
	}

	if(!PyArg_ParseTuple(args, "OOii:clone", &ctx->func, &ctx->args, &flags, &child_stack_size)) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to parse fett.clone() arguments");
		return NULL;
	}

	child_stack = (void **)(malloc(child_stack_size) + (child_stack_size / sizeof(*child_stack)) - 1);
	if(child_stack == NULL) {
		PyErr_SetString(PyExc_MemoryError, "Unable to allocate memory for child stack");
		return NULL;
	}

	Py_INCREF(ctx->func);
	Py_INCREF(ctx->args);

	pid = clone(fett_clone_callback, child_stack, flags, ctx);
	if(pid == -1) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	return Py_BuildValue("i", pid);
}


static PyObject *fett_unshare(PyObject *self, PyObject *args) {
	int flags;

	if(!PyArg_ParseTuple(args, "i:unshare", &flags)) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to parse fett.unshare() arguments");
		return NULL;
	}

	if(unshare(flags)) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}
	return Py_BuildValue("");
}


static PyObject *fett_sethostname(PyObject *self, PyObject *args) {
	const char *hostname;
	Py_ssize_t len;

	if(!PyArg_ParseTuple(args, "s#:sethostname", &hostname, &len)) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to parse fett.sethostname() arguments");
		return NULL;
	}

	if(sethostname(hostname, len)) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}
	return Py_BuildValue("");
}


static PyObject *fett_mount(PyObject *self, PyObject *args) {
	const char *source;
	const char *target;
	const char *filesystemtype;
	unsigned long mountflags;
	const void *data;

	if(!PyArg_ParseTuple(args, "sssks:mount", &source, &target, &filesystemtype, &mountflags, &data)) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to parse fett.mount() arguments");
		return NULL;
	}

	if(mount(source, target, filesystemtype, mountflags, data)) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	return Py_BuildValue("");
}


static PyObject *fett_umount(PyObject *self, PyObject *args) {
	const char *target;

	if(!PyArg_ParseTuple(args, "s:umount", &target)) {
		PyErr_SetString(PyExc_RuntimeError, "Unable to parse fett.umount() arguments");
		return NULL;
	}

	if(umount(target)) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	return Py_BuildValue("");
}


static PyMethodDef FettMethods[] = {
	{"clone", fett_clone, METH_VARARGS, "See clone(2)"},
	{"unshare", fett_unshare, METH_VARARGS, "See unshare(2)"},
	{"sethostname", fett_sethostname, METH_VARARGS, "See sethostname(2)"},
	{"mount", fett_mount, METH_VARARGS, "See mount(2)"},
	{"umount", fett_umount, METH_VARARGS, "See umount(2)"},
	{NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC initfett(void) {
	PyObject *module;

	module = Py_InitModule("fett", FettMethods);

	// clone flags
	PyModule_AddIntConstant(module, "CLONE_CHILD_CLEARTID",	CLONE_CHILD_CLEARTID);
	PyModule_AddIntConstant(module, "CLONE_CHILD_SETTID",	CLONE_CHILD_SETTID);
	PyModule_AddIntConstant(module, "CLONE_FILES",			CLONE_FILES);
	PyModule_AddIntConstant(module, "CLONE_FS",				CLONE_FS);
	PyModule_AddIntConstant(module, "CLONE_IO",				CLONE_IO);
	PyModule_AddIntConstant(module, "CLONE_NEWIPC",			CLONE_NEWIPC);
	PyModule_AddIntConstant(module, "CLONE_NEWNET",			CLONE_NEWNET);
	PyModule_AddIntConstant(module, "CLONE_NEWNS",			CLONE_NEWNS);
	PyModule_AddIntConstant(module, "CLONE_NEWPID",			CLONE_NEWPID);
	PyModule_AddIntConstant(module, "CLONE_NEWUTS",			CLONE_NEWUTS);
	PyModule_AddIntConstant(module, "CLONE_PARENT",			CLONE_PARENT);
	PyModule_AddIntConstant(module, "CLONE_PARENT_SETTID",	CLONE_PARENT_SETTID);
	PyModule_AddIntConstant(module, "CLONE_PTRACE",			CLONE_PTRACE);
	PyModule_AddIntConstant(module, "CLONE_SETTLS",			CLONE_SETTLS);
	PyModule_AddIntConstant(module, "CLONE_SIGHAND",		CLONE_SIGHAND);
	PyModule_AddIntConstant(module, "CLONE_SYSVSEM",		CLONE_SYSVSEM);
	PyModule_AddIntConstant(module, "CLONE_THREAD",			CLONE_THREAD);
	PyModule_AddIntConstant(module, "CLONE_UNTRACED",		CLONE_UNTRACED);
	PyModule_AddIntConstant(module, "CLONE_VFORK",			CLONE_VFORK);
	PyModule_AddIntConstant(module, "CLONE_VM",				CLONE_VM);

	PyModule_AddIntConstant(module, "SIGCHLD",				SIGCHLD);

	// mount flags
	PyModule_AddIntConstant(module, "MS_BIND",				MS_BIND);
	PyModule_AddIntConstant(module, "MS_DIRSYNC",			MS_DIRSYNC);
	PyModule_AddIntConstant(module, "MS_MANDLOCK",			MS_MANDLOCK);
	PyModule_AddIntConstant(module, "MS_MOVE",				MS_MOVE);
	PyModule_AddIntConstant(module, "MS_NOATIME",			MS_NOATIME);
	PyModule_AddIntConstant(module, "MS_NODEV",				MS_NODEV);
	PyModule_AddIntConstant(module, "MS_NODIRATIME",		MS_NODIRATIME);
	PyModule_AddIntConstant(module, "MS_NOEXEC",			MS_NOEXEC);
	PyModule_AddIntConstant(module, "MS_NOSUID",			MS_NOSUID);
	PyModule_AddIntConstant(module, "MS_RDONLY",			MS_RDONLY);
	PyModule_AddIntConstant(module, "MS_RELATIME",			MS_RELATIME);
	PyModule_AddIntConstant(module, "MS_REMOUNT",			MS_REMOUNT);
	PyModule_AddIntConstant(module, "MS_SILENT",			MS_SILENT);
	PyModule_AddIntConstant(module, "MS_STRICTATIME",		MS_STRICTATIME);
	PyModule_AddIntConstant(module, "MS_SYNCHRONOUS",		MS_SYNCHRONOUS);
}
