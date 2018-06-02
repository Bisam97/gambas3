#include <math.h>

#define NORETURN __attribute__((noreturn))

#define E_OVERFLOW   7
#define E_NOBJECT   12
#define E_BOUND     21


typedef
  unsigned char uchar;

typedef
  GB_VALUE VALUE;

typedef
  ushort PCODE;

typedef
	void (*EXEC_FUNC)();

typedef
	void (*EXEC_FUNC_CODE)(ushort);

typedef
  struct {
    GB_TYPE type;
    void *class;
    void *super;
    }
  GB_VALUE_CLASS;
  
typedef
  struct {
    GB_TYPE type;
    void *addr;
    void *gp;
  }
  GB_VALUE_GOSUB;
  
typedef
	struct {
		GB_BASE object;
		int size;
		int count;
		GB_TYPE type;
		void *data;
		int *dim;
		void *ref;
		}
	GB_ARRAY_IMPL;

#define SP (*(JIT.sp))
#define PC (*(JIT.pc))
#define CP (*(JIT.cp))
#define OP (*(JIT.op))

#define GET_STRING(_addr, _start, _len) ({ \
  GB_STRING temp; \
  temp.type = GB_T_STRING; \
  temp.value.addr = (char *)(_addr); \
  temp.value.start = (_start); \
  temp.value.len = (_len); \
  temp; })

#define GET_CSTRING(_addr, _start, _len) ({ \
  GB_STRING temp; \
  temp.type = GB_T_CSTRING; \
  temp.value.addr = (char *)(_addr); \
  temp.value.start = (_start); \
  temp.value.len = (_len); \
  temp; })
  
#define GET_OBJECT(_addr, _type) ({ \
  GB_OBJECT temp; \
  temp.type = (GB_TYPE)(_type); \
  temp.value = (_addr); \
  temp; })

#define CHECK_FINITE(_val) (isfinite(_val) ? (_val) : JIT.throw(E_OVERFLOW))

#define PARAM_b(_p) (sp[-n + (_p)]._boolean.value)
#define PARAM_c(_p) (sp[-n + (_p)]._byte.value)
#define PARAM_h(_p) (sp[-n + (_p)]._short.value)
#define PARAM_i(_p) (sp[-n + (_p)]._integer.value)
#define PARAM_l(_p) (sp[-n + (_p)]._long.value)
#define PARAM_g(_p) (sp[-n + (_p)]._single.value)
#define PARAM_f(_p) (sp[-n + (_p)]._float.value)
#define PARAM_p(_p) (sp[-n + (_p)]._pointer.value)
#define PARAM_d(_p) (*(GB_DATE *)&sp[-n + (_p)])
#define PARAM_s(_p) (*(GB_STRING *)&sp[-n + (_p)])
#define PARAM_o(_p) (*(GB_OBJECT *)&sp[-n + (_p)])

#define RETURN_b(_val) (GB.ReturnBoolean(_val))
#define RETURN_c(_val) (GB.Return(GB_T_BYTE, (int)(_val)))
#define RETURN_h(_val) (GB.Return(GB_T_SHORT, (int)(_val)))
#define RETURN_i(_val) (GB.ReturnInteger(_val))
#define RETURN_l(_val) (GB.ReturnLong(_val))
#define RETURN_g(_val) (GB.ReturnSingle(_val))
#define RETURN_f(_val) (GB.ReturnFloat(_val))
#define RETURN_d(_val) ({ GB_DATE _v = (_val); GB.ReturnDate(&_v); })
#define RETURN_p(_val) (GB.ReturnPointer(_val))
#define RETURN_o(_val) (GB.ReturnObject((_val).value))

#define PUSH_b(_val) ({ char _v = -(_val); sp->_boolean.value = v; sp->type = GB_T_BOOLEAN; sp++; })
#define PUSH_c(_val) ({ uchar _v = (_val); sp->_byte.value = _v; sp->type = GB_T_BYTE; sp++; })
#define PUSH_h(_val) ({ short _v = (_val); sp->_short.value = _v; sp->type = GB_T_SHORT; sp++; })
#define PUSH_i(_val) ({ int _v = (_val); sp->_integer.value = _v; sp->type = GB_T_INTEGER; sp++; })
#define PUSH_l(_val) ({ int64_t _v = (_val); sp->_long.value = _v; sp->type = GB_T_LONG; sp++; })
#define PUSH_g(_val) ({ float _v = (_val); sp->_single.value = _v; sp->type = GB_T_SINGLE; sp++; })
#define PUSH_f(_val) ({ double _v = (_val); sp->_float.value = _v; sp->type = GB_T_FLOAT; sp++; })
#define PUSH_p(_val) ({ intptr_t _v = (_val); sp->_pointer.value = _v; sp->type = GB_T_POINTER; sp++; })
#define PUSH_d(_val) (*((GB_DATE *)sp) = (_val), sp++)
#define PUSH_t(_val) (*((GB_STRING *)sp) = (_val), sp++)
#define PUSH_s(_val) ({ GB_STRING _v = (_val); GB.RefString(_v.value.addr); *((GB_STRING *)sp) = _v; sp++; })
#define PUSH_o(_val) ({ GB_OBJECT _v = (_val); GB.Ref(_v.value); *((GB_OBJECT *)sp) = _v; sp++; })
#define PUSH_C(_val) ({ GB_VALUE_CLASS _v; _v.type = GB_T_CLASS; _v.class = (_val); *((GB_VALUE_CLASS *)sp) = _v; sp++; })
#define PUSH_u(_val) (*sp = (_val), GB.BorrowValue(sp), sp++)

#define POP_b() (sp--, sp->_boolean.value)
#define POP_c() (sp--, sp->_byte.value)
#define POP_h() (sp--, sp->_short.value)
#define POP_i() (sp--, sp->_integer.value)
#define POP_l() (sp--, sp->_long.value)
#define POP_g() (sp--, sp->_single.value)
#define POP_f() (sp--, sp->_float.value)
#define POP_p() (sp--, sp->_pointer.value)
#define POP_d() (sp--, *((GB_DATE*)sp))
#define POP_s() (sp--, JIT.unborrow(sp), *((GB_STRING*)sp))
#define POP_o() (sp--, JIT.unborrow(sp), *((GB_OBJECT*)sp))
#define POP_t() (sp--, *((GB_STRING*)sp))
#define POP_u() (sp--, JIT.unborrow(sp), *sp)

#define BORROW_s(_val) ({ GB_STRING _v = (_val); GB.RefString(_v.value.addr); _v; })
#define BORROW_o(_val) ({ GB_OBJECT _v = (_val); GB.Ref(_v.value); _v; })

#define RELEASE_s(_val) ({ GB_STRING _v = (_val); GB.FreeString(&_v.value.addr); _v; })
#define RELEASE_o(_val) ({ GB_OBJECT _v = (_val); GB.Unref(&_v.value); _v; })

#define GET_CLASS(_index) (JIT.get_class_ref(_index))

#define CONSTANT_l(_index) ({ \
  JIT_CONSTANT *_cc = JIT.get_constant(_index); \
  _cc->_long.value; \
})

#define CONSTANT_g(_index) ({ \
  JIT_CONSTANT *_cc = JIT.get_constant(_index); \
  _cc->_single.value; \
})

#define CONSTANT_f(_index) ({ \
  JIT_CONSTANT *_cc = JIT.get_constant(_index); \
  _cc->_float.value; \
})

#define CONSTANT_s(_index) ({ \
  JIT_CONSTANT *_cc = JIT.get_constant(_index); \
  GET_CSTRING(_cc->_string.addr, 0, _cc->_string.len); \
})

#define CONSTANT_t(_index) ({ \
  JIT_CONSTANT *_cc = JIT.get_constant(_index); \
  GET_CSTRING(GB.Translate(_cc->_string.addr), 0, strlen(temp.value.addr)); \
})

#define GET_CHAR(_char) GET_CSTRING(&JIT.char_table[(_char) * 2], 0, 1)
  
#define CALL_SUBR(_pc, _code) (PC = &pc[_pc], SP = sp, (*(EXEC_FUNC)JIT.subr_table[(_code) >> 8])(), sp = SP)
#define CALL_SUBR_CODE(_pc, _code) (PC = &pc[_pc], SP = sp, (*(EXEC_FUNC_CODE)JIT.subr_table[(_code) >> 8])(_code), sp = SP)

#define CALL_NEW(_pc, _code) (PC = &pc[_pc], SP = sp, (*(EXEC_FUNC)JIT.new)(), sp = SP)
#define CALL_PUSH_ARRAY(_pc, _code) (PC = &pc[_pc], SP = sp, (*(EXEC_FUNC)JIT.push_array)(_code), sp = SP)
#define CALL_POP_ARRAY(_pc, _code) (PC = &pc[_pc], SP = sp, (*(EXEC_FUNC)JIT.pop_array)(_code), sp = SP, sp++)

#define GET_STATIC(_index)  JIT.get_static_addr(_index)

#define GET_STATIC_b(_index) (*(bool *)GET_STATIC(_index))
#define GET_STATIC_c(_index) (*(uchar *)GET_STATIC(_index))
#define GET_STATIC_h(_index) (*(short *)GET_STATIC(_index))
#define GET_STATIC_i(_index) (*(int *)GET_STATIC(_index))
#define GET_STATIC_l(_index) (*(int64_t *)GET_STATIC(_index))
#define GET_STATIC_g(_index) (*(float *)GET_STATIC(_index))
#define GET_STATIC_f(_index) (*(double *)GET_STATIC(_index))
#define GET_STATIC_p(_index) (*(intrptr_t *)GET_STATIC(_index))
#define GET_STATIC_s(_index) GET_STRING((*(char **)GET_STATIC(_index)), 0, GB.StringLength(temp.value.addr))
#define GET_STATIC_o(_index, _type) GET_OBJECT((*(char **)GET_STATIC(_index)), _type)

#define SET_STATIC_b(_index, _val) (GET_STATIC_b(_index) = (_val))
#define SET_STATIC_c(_index, _val) (GET_STATIC_c(_index) = (_val))
#define SET_STATIC_h(_index, _val) (GET_STATIC_h(_index) = (_val))
#define SET_STATIC_i(_index, _val) (GET_STATIC_i(_index) = (_val))
#define SET_STATIC_l(_index, _val) (GET_STATIC_l(_index) = (_val))
#define SET_STATIC_g(_index, _val) (GET_STATIC_g(_index) = (_val))
#define SET_STATIC_f(_index, _val) (GET_STATIC_f(_index) = (_val))
#define SET_STATIC_p(_index, _val) (GET_STATIC_p(_index) = (_val))
#define SET_STATIC_s(_index, _val) ({ GB_VALUE temp = (GB_VALUE)(_val); GB.Store(GB_T_STRING, &temp, GET_STATIC(_index)); })
#define SET_STATIC_o(_index, _val) ({ GB_VALUE temp = (GB_VALUE)(_val); GB.Store(GB_T_OBJECT, &temp, GET_STATIC(_index)); })

#define GET_DYNAMIC(_index)  JIT.get_dynamic_addr(_index)

#define GET_DYNAMIC_b(_index) *(bool *)GET_DYNAMIC(_index)
#define GET_DYNAMIC_c(_index) *(uchar *)GET_DYNAMIC(_index)
#define GET_DYNAMIC_h(_index) *(short *)GET_DYNAMIC(_index)
#define GET_DYNAMIC_i(_index) *(int *)GET_DYNAMIC(_index)
#define GET_DYNAMIC_l(_index) *(int64_t *)GET_DYNAMIC(_index)
#define GET_DYNAMIC_g(_index) *(float *)GET_DYNAMIC(_index)
#define GET_DYNAMIC_f(_index) *(double *)GET_DYNAMIC(_index)
#define GET_DYNAMIC_p(_index) *(intptr_t *)GET_DYNAMIC(_index)
#define GET_DYNAMIC_s(_index) GET_STRING((*(char **)GET_DYNAMIC(_index)), 0, GB.StringLength(temp.value.addr))

#define SET_DYNAMIC_b(_index, _val) ((bool *)GET_DYNAMIC(_index) = (_val))
#define SET_DYNAMIC_c(_index, _val) ((uchar *)GET_DYNAMIC(_index) = (_val))
#define SET_DYNAMIC_h(_index, _val) ((short *)GET_DYNAMIC(_index) = (_val))
#define SET_DYNAMIC_i(_index, _val) ((int *)GET_DYNAMIC(_index) = (_val))
#define SET_DYNAMIC_l(_index, _val) ((int64_t *)GET_DYNAMIC(_index) = (_val))
#define SET_DYNAMIC_g(_index, _val) ((float *)GET_DYNAMIC(_index) = (_val))
#define SET_DYNAMIC_f(_index, _val) ((double *)GET_DYNAMIC(_index) = (_val))
#define SET_DYNAMIC_p(_index, _val) ((intptr_t *)GET_DYNAMIC(_index) = (_val))
#define SET_DYNAMIC_s(_index, _val) ({ GB_VALUE temp = (GB_VALUE)(_val); GB.Store(GB_T_STRING, &temp, GET_DYNAMIC(_index)); })

#define GET_ARRAY_NO_CHECK(_type, _array, _index) ({ \
  GB_ARRAY_IMPL *_a = (_array).value; \
  int _i = (_index); \
  &((_type *)_a->data)[_i]; \
})

#define GET_ARRAY(_type, _array, _index) ({ \
  GB_ARRAY_IMPL *_a = (_array).value; \
  int _i = (_index); \
  if (!_a) JIT.throw(E_NOBJECT); \
  if (_i < 0 || _i > _a->count) JIT.throw(E_BOUND); \
  &((_type *)_a->data)[_i]; \
})

#define PUSH_ARRAY(_type, _array, _index) *GET_ARRAY(_type, _array, _index)

#define PUSH_ARRAY_b(_array, _index) PUSH_ARRAY(bool, _array, _index)
#define PUSH_ARRAY_c(_array, _index) PUSH_ARRAY(uchar, _array, _index)
#define PUSH_ARRAY_h(_array, _index) PUSH_ARRAY(short, _array, _index)
#define PUSH_ARRAY_i(_array, _index) PUSH_ARRAY(int, _array, _index)
#define PUSH_ARRAY_l(_array, _index) PUSH_ARRAY(int64_t, _array, _index)
#define PUSH_ARRAY_g(_array, _index) PUSH_ARRAY(float, _array, _index)
#define PUSH_ARRAY_f(_array, _index) PUSH_ARRAY(double, _array, _index)
#define PUSH_ARRAY_p(_array, _index) PUSH_ARRAY(intptr_t, _array, _index)
#define PUSH_ARRAY_s(_array, _index) GET_STRING(PUSH_ARRAY(char *, _array, _index), 0, GB.StringLength(temp.value.addr))

#define POP_ARRAY(_type, _array, _index, _val) (*GET_ARRAY(_type, _array, _index) = (_val))

#define POP_ARRAY_b(_array, _index, _val) POP_ARRAY(bool, _array, _index, _val)
#define POP_ARRAY_c(_array, _index, _val) POP_ARRAY(uchar, _array, _index, _val)
#define POP_ARRAY_h(_array, _index, _val) POP_ARRAY(short, _array, _index, _val)
#define POP_ARRAY_i(_array, _index, _val) POP_ARRAY(int, _array, _index, _val)
#define POP_ARRAY_l(_array, _index, _val) POP_ARRAY(int64_t, _array, _index, _val)
#define POP_ARRAY_g(_array, _index, _val) POP_ARRAY(float, _array, _index, _val)
#define POP_ARRAY_f(_array, _index, _val) POP_ARRAY(double, _array, _index, _val)
#define POP_ARRAY_p(_array, _index, _val) POP_ARRAY(intptr_t, _array, _index, _val)

#define POP_ARRAY_s(_array, _index, _val) ({ GB_VALUE temp = (GB_VALUE)(_val); GB.Store(GB_T_STRING, &temp, GET_ARRAY(char *, _array, _index)); })

#define CONV_d_b(_val) ({ GB_DATE _v = (_val); _v.value.date != 0 || _v.value.time != 0; })
#define CONV_d_c(_val) ((uchar)((_val).value.date))
#define CONV_d_h(_val) ((short)((_val).value.date))
#define CONV_d_i(_val) ((_val).value.date)
#define CONV_d_l(_val) ((int64_t)((_val).value.date))
#define CONV_d_g(_val) ({ GB_DATE _v = (_val); (float)((float)_v.value.date + (float)_v.value.time / 86400000.0); })
#define CONV_d_f(_val) ({ GB_DATE _v = (_val); (double)((double)_v.value.date + (double)_v.value.time / 86400000.0); })
#define CONV_d_p(_val) (JIT.throw(E_CONV))
#define CONV_d_s(_val) (PUSH_d(_val),JIT.conv(sp - 1, GB_T_STRING),POP_s())
#define CONV_d_o(_val) (JIT.throw(E_CONV))

#define CONV_u_b(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_BOOLEAN),POP_b())
#define CONV_u_c(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_BYTE),POP_c())
#define CONV_u_h(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_SHORT),POP_h())
#define CONV_u_i(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_INTEGER),POP_i())
#define CONV_u_l(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_LONG),POP_l())
#define CONV_u_g(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_SINGLE),POP_g())
#define CONV_u_f(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_FLOAT),POP_f())
#define CONV_u_d(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_DATE),POP_d())
#define CONV_u_p(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_POINTER),POP_p())
#define CONV_u_s(_val) (PUSH_u(_val),JIT.conv(sp - 1, GB_T_STRING),POP_s())
#define CONV_u_O(_val, _class) (PUSH_u(_val),JIT.conv(sp - 1,GET_CLASS(_class)),POP_o())

#define CONV_o_O(_val, _class) (PUSH_o(_val),JIT.conv(sp - 1,GET_CLASS(_class)),POP_o())

#define PUSH_GOSUB(_label) ({ \
  GB_VALUE_GOSUB *_p = (GB_VALUE_GOSUB *)sp; \
  _p->type = GB_T_VOID; \
  _p->addr = &&_label; \
  _p->gp = gp; \
  gp = _p; \
  sp++; \
})

#define RETURN_GOSUB() ({ \
  void *_addr; \
  if (!gp) goto __RETURN; \
  _addr = gp->addr; \
  sp = (GB_VALUE *)gp; \
  gp = gp->gp; \
  goto *_addr; \
})

#define PUSH_UNKNOWN(_pc) (PC = &pc[_pc], SP = sp, JIT.push_unknown(), sp = SP, POP_u())
#define POP_UNKNOWN(_pc) (PC = &pc[_pc], SP = sp, JIT.pop_unknown(), sp = SP)

#define GET_ME_STATIC() (CP)
#define GET_ME() ({ GB_OBJECT _v; _v.type = CP; _v.value = OP; _v; })

#define CALL_UNKNOWN(_pc) (JIT.call_unknown(&pc[_pc], sp), sp = SP, POP_u())

