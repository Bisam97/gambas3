static VALUE *_jit_print_catch(VALUE **psp, VALUE *sp, VALUE *ssp, void *cp, void *fp, bool has_catch_finally)
{
  CP = cp;
  FP = fp;
  if (has_catch_finally)
    JIT.error_set_last(FALSE);
  if (SP > sp) sp = SP; else SP = sp;
  LEAVE_SUPER();
  if (sp > ssp) { JIT.release_many(sp, sp - ssp); SP = sp = ssp; }
  return sp;
}

static VALUE *_jit_end_try(VALUE **psp, VALUE *sp)
{
  if (SP > sp) sp = SP; else SP = sp;
  LEAVE_SUPER();
  if (sp > EP) { JIT.release_many(sp, sp - EP); SP = sp = EP; }
  JIT.set_got_error(1);
  JIT.error_set_last(FALSE);
  return sp;
}