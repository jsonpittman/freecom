/* $Id$
 *	Module implementation for Stub
 *
 * 2000/11/10 ska
 * started
 */

#include <assert.h>
#include <dos.h>
#include <limits.h>
#include <stdarg.h>
#include <mcb.h>

#include "config.h"
#include "debug.h"
#include "module.h"
typedef void interrupt (*isr)();

#include "stub.ver"		/* module's version */
#error "Generated by an external tool from 'stub' containing various offsets"
#include "stub.var"		/* module offsets */

static MOD_LOADSTATE_STRUCT(mod_state_stub_t)
	word anchessor;
} state;

static const char modToken[] = "STUB";

static void activateStubModule(void)
{	Chain the current module into the system
}

static void duplicateModule(word srcSeg)
{
	word dstSegm;

	src = MK_FP(srcSegm, 0);
	assert(src->mcb_size < UINT_MAX / 16);
	if((dstSegm = allocBlk(src->mcb_size, 0x82)) != 0) {
		_fmemcpy(MK_FP(dstSegm, 0), src, src->mcb_size * 16);
		patchModuleMCB(dstSegm);
		state.segm = dstSegm;
		state.mode = MOD_LOADED;
	}
}

static int isStubModule(MIB far*mib)
{
	return mib->mib_majorID == RES_ID_STUB
	 && mib->mib_minorID == 0	/* reload from file */
	 && mib->mib_version == MOD_VERSION;
}

int getAnchessor(void *arg, word segm)
{
	MIBp mib;

	if(lockModule(segm)) {	/* is a real module */
		mib = MCBtoMIB(segm);	/* verify it's the correct module */
		if(isStubModule(mib)) {
			duplicateStubModule(mib);
			if(!arg) sessionInheritDefaults(MIB2CONTEXT(mib));
			activateStubModule();
			state.mode = MOD_ATTACHED;
			unlockModule(segm);
		 	return segm;		/* stop cycle */
		}
		unlockModule(segm);
	}
	return 0;	/* proceed searching */
}

#pragma argsused
static int attachToModule(void *arg, word segm)
{	if(lockModule(segm)) {
		if(state.mode != MOD_LOADED && isStubModule(MCB2MIB(segm))) {
			duplicateModule(segm);
		} else {
			modCriter(MOD_ATTACH_TO_SEGM, segm);
			modCBreak(MOD_ATTACH_TO_SEGM, segm);
			modRspFailed(MOD_ATTACH_TO_SEGM, segm);
		}
		unlockModule(segm);
	}

	return 0;	/* proceed */
}

#pragma argsused
static int loadModule(res_majorid_t majorid
	, res_minorid_t minorid
	, res_version_t version
	, long length
	, FILE *f
	, void *arg)
{	modCriter(MOD_LOAD_FROM_RESOURCE_FILE, majorid, minorid, version, length, f, arg);
	modCBreak(MOD_LOAD_FROM_RESOURCE_FILE, majorid, minorid, version, length, f, arg);
	modRspFailed(MOD_LOAD_FROM_RESOURCE_FILE, majorid, minorid, version, length, f, arg);
	modStub(MOD_LOAD_FROM_RESOURCE_FILE, majorid, minorid, version, length, f, arg);
	return 0;		/* Proceed scanning */
}


static void getFreeCOMRoots(void)
{	word segm;
	context_t far* context;
	MIBp mib;

	/* The stub module is not really a shared module, there is
		exactly one module per FreeCOM instance.
		Er, perhaps not in case of INT-2E -- 2000/11/10 ska*/
	segm = peekw(_psp, 0x16) - 1;		/* parent process MCB */
	if(lockModule(segm)) {	/* is a real module */
		mib = MCBtoMIB(segm);	/* verify it's the correct module */
		if(isStubModule(mib)) {
			/* Check if this copy of FreeCOM was _respawned_
				by this stub module */
			context = MIB2CONTEXT(mib);
			if(context->respawn.exitReason != -1) {
				/* This is the real stub for this FreeCOM instance */
				attachModule(segm);
				state.mib = mib;
				state.mode = MOD_ATTACHED;
			}
		}
#ifdef DEBUG
		else if(mib->mib_majorID == RES_ID_STUB)
			dprintf(("[STUB incompatible module found @0x%04x: minor=%u, version=%u]\n"
		 , segm, mib->mib_minorID, mib->mib_version));
#endif
		unlockModule(segm);
	}
}

int modStub(mod_state_t action, ...)
{	va_list ap;
	word segm;
	MIB far *mib;
	context_t far *context;

	va_start(ap, action);

	switch(action) {
	case MOD_INIT:	/* search for the module to attach to */
		if(state.mode != MOD_ATTACHED) {
			getFreeCOMRoots();
		}
		break;

	case MOD_PRELOAD:
		if(state.mode != MOD_ATTACHED
			/* Search for anchessors from which we could inherit default
				settings */
		  && mcb_allParents(SEG2MCB(_psp), getAnchessor, 0) == 0) {
		  	/* Try to search any other FreeCOM instance, but don#('t
		  		inherit its settings */
		  	mcb_forAll(0, getAnchessor, (void*)1);
		}
		break;

	case MOD_LOAD:			/* load from resource file or inherit from
								anchessor */
		if(state.mode != MOD_ATTACHED) {
			if(state.anchessor) {
				/* Duplicate the anchessor and re-use its modules */
				segm = dupModule(state.anchessor, MOD_STUB_ALLOC_MODE);
				if(segm) {
					/* re-use its modules */
					modCriter(MOD_ATTACH, peekw(segm + 2, stubCriterJmpSeg));
					modCBreak(MOD_ATTACH, peekw(segm + 2, stubCBreakJmpSeg));
					modRspFailed(MOD_ATTACH, peekw(segm + 2, stubRspFailedJmpSeg));
					goto joinStub;
				}

				state.anchessor = 0;
			}
			/* Try to attach the other modules to any loaded into
				memory. */
			mcb_forAll(0, attachToModule, 0);
				/* for optmiziation purpose:
					if all modules are loaded, the resources are not
					scanned. */
			if(state.mode != MOD_LOADED) {
				/* Implicit dependecy: If out type of the Stub module is
					already loaded into memory, out types of the other
					modules must be attached to already */
				enumResources(0, loadModule, 0);
			}
				/* Chain the module into the process tree */
			if(state.mode == MOD_LOADED
			 && (modCriter = modCrit(MOD_LOADED)) != 0
			 && (modCBreak = modCBreak(MOD_LOADED)) != 0
			 && (modRspFailed = modRspFailed(MOD_LOADED)) != 0) {
			 	context_t far*context;
			 	word modCriter, modCBreak, modRspFailed;

			 	assert(state.segm > 0x40);
			 	state.mib = mib = MCB2MIB(state.segm);
			 	attachModule(state.segm);
			 	context = MIB2CONTEXT(mib);
			 	segm = FP_SEG(context);		/* Stub segment */
			 	/* Update references */
			 	if(lockModule(modCriter)) {
			 		attachModule(modCriter);
					pokew(segm, context->stub_params.ref_Criter + 2
					 , modCriter);
					unlockModule(modCriter);
				}
			 	if(lockModule(modCBreak)) {
			 		attachModule(modCBreak);
					pokew(segm, context->stub_params.ref_CBreak + 2
					 , modCBreak);
					unlockModule(modCBreak);
				}
			 	if(lockModule(modRspFailed)) {
			 		attachModule(modRspFailed);
					pokew(segm, context->stub_params.ref_RspFailed + 2
					 , modRspFailed);
					unlockModule(modRspFailed);
				}
			}

				 	/* Cannot attach */
				 	deallocModule(state.segm);
				 	state.mode = MOD_INVALID;
				 	return 0;
				}
		}
		break;

	case MOD_LOAD_FROM_RESOURCE_FILE:
			/* first try to duplicate an anchessor */
			FILE *f = va_arg(ap, FILE*);
			res_length_t length = va_arg(ap, res_length_t);
			res_majorid_t major = va_arg(ap, res_majorid_t);
			res_minorid_t minor = va_arg(ap, res_minorid_t);
			res_version_t version = va_arg(ap, res_version_t);

			if(major == RES_ID_STUB
			 && minor == 0	/* reload from file */
			 && version == MOD_VERSION
			 && (mib = loadSimpleModule(f, length, major, minor, version
			             , modToken)) != 0)
				closeIntModule(mib, (mod_load_state_t*)&state, modToken
				 , *(void far**)MK_FP(_psp, 0xe));		/* original INT23 */
#ifdef DEBUG
			else if(major == RES_ID_STUB)
				dprintf(("[STUB incompatible resource found @%lu: minor=%u, version=%u]\n"
				 , length, minor, version));
#endif
		}
		break;

	case MOD_LOAD_DEFAULTS:		/* from anchessor */
		break;

	case MOD_ATTACHED:
		if(state.mode != MOD_ATTACHED)
			return 0;
		return MIBtoMCB(state.mib);	

	case MOD_LOADED:
		if(state.mode != MOD_LOADED && state.mode != MOD_ATTACHED)
			return 0;
		return MIBtoMCB(state.mib);	

#ifndef NDEBUG
	default:
		fprintf(stderr, "Assertation failed: Invalid action code in CBREAK: %u\n", action);
#endif
	}

	return 1;
}
