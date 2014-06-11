
#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
  #include <winsock.h>
  #include <errno.h>
  #include <io.h>
  #include <process.h>
  #define mode_t int
#else
  #include <err.h>
  #include <unistd.h>
#endif

#include <setjmp.h>

#include "mruby.h"
#include "mruby/compile.h"
#include "mruby/dump.h"
#include "mruby/string.h"
#include "mruby/proc.h"

#include "opcode.h"
#include "error.h"

#include <stdlib.h>
#include <sys/stat.h>

#define E_LOAD_ERROR (mrb_class_get(mrb, "LoadError"))

#if MRUBY_RELEASE_NO < 10000
mrb_value mrb_yield_internal(mrb_state *mrb, mrb_value b, int argc, mrb_value *argv, mrb_value self, struct RClass *c);
#define mrb_yield_with_class mrb_yield_internal
#endif

static void
replace_stop_with_return(mrb_state *mrb, mrb_irep *irep)
{
  if (irep->iseq[irep->ilen - 1] == MKOP_A(OP_STOP, 0)) {
    irep->iseq = mrb_realloc(mrb, irep->iseq, (irep->ilen + 1) * sizeof(mrb_code));
    irep->iseq[irep->ilen - 1] = MKOP_A(OP_LOADNIL, 0);
    irep->iseq[irep->ilen] = MKOP_AB(OP_RETURN, 0, OP_R_NORMAL);
    irep->ilen++;
  }
}

static void
eval_load_irep(mrb_state *mrb, mrb_irep *irep)
{
  int ai;
  struct RProc *proc;

  replace_stop_with_return(mrb, irep);
  proc = mrb_proc_new(mrb, irep);
  proc->target_class = mrb->object_class;

  ai = mrb_gc_arena_save(mrb);
  mrb_yield_with_class(mrb, mrb_obj_value(proc), 0, NULL, mrb_top_self(mrb), mrb->object_class);
  mrb_gc_arena_restore(mrb, ai);
}

static mrb_value
mrb_require_load_rb_str(mrb_state *mrb, mrb_value self)
{
  int fd = -1, ret;
  mrb_irep *irep;
  mrb_value code, path = mrb_nil_value();

  mrb_get_args(mrb, "S|S", &code, &path);
  if (!mrb_string_p(path)) {
    path = mrb_str_new_cstr(mrb, "-");
  }

  return mrb_load_nstring(mrb, RSTRING_PTR(code), RSTRING_LEN(code));
}

static mrb_value
mrb_require_load_mrb_file(mrb_state *mrb, mrb_value self)
{
  char *path_ptr = NULL;
  int fd = -1, ret;
  mrb_irep *irep;
  mrb_value code, path = mrb_nil_value();

  mrb_get_args(mrb, "S|S", &code, &path);
  if (!mrb_string_p(path)) {
    path = mrb_str_new_cstr(mrb, "-");
  }
  path_ptr = mrb_str_to_cstr(mrb, path);

  irep = mrb_read_irep(mrb, RSTRING_PTR(code));

  if (irep) {
    eval_load_irep(mrb, irep);
  } else if (mrb->exc) {
    // fail to load
    longjmp(*(jmp_buf*)mrb->jmp, 1);
  } else {
    mrb_raisef(mrb, E_LOAD_ERROR, "can't load file -- %S", path);
    return mrb_nil_value();
  }

  return mrb_true_value();
}

void
mrb_mruby_require_gem_init(mrb_state *mrb)
{
  struct RClass *krn;
  krn = mrb->kernel_module;

  mrb_define_method(mrb, krn, "_load_rb_str",   mrb_require_load_rb_str,   MRB_ARGS_ANY());
  mrb_define_method(mrb, krn, "_load_mrb_file", mrb_require_load_mrb_file, MRB_ARGS_ANY());
}

void
mrb_mruby_require_gem_final(mrb_state *mrb)
{
}
