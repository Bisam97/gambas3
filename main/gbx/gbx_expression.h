/***************************************************************************

  gbx_expression.h

  (c) 2000-2009 Benoît Minisini <gambas@users.sourceforge.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

***************************************************************************/

#ifndef __GBX_EXPRESSION_H
#define __GBX_EXPRESSION_H

#include "gb_table.h"
#include "gbx_class.h"

typedef
  struct {
    char *source;
    int len;
    PATTERN *pattern;
    int pattern_count;
    PATTERN *current;
    PATTERN *tree;
    CLASS exec_class;
    CLASS_LOAD class_load;
    FUNCTION func;
    CLASS_CONST *cst;
    ushort *code;
    ushort ncode;
    ushort ncode_max;
    TABLE *table;
    TABLE *string;
    /*TABLE *variable;*/
    CLASS **class;
    char **unknown;
    int *var;
    short nvar;
    short last_code;
    short last_code2;
    unsigned analyze : 1;
    unsigned rewrite : 1;
		unsigned comment : 1;
    unsigned _reserved : 13;
    int stack_usage;
    void *op;
    }
  PACKED
  EXPRESSION;

typedef
  struct {
    SYMBOL sym;
    int local;
    }
  EVAL_SYMBOL;
  
#endif
