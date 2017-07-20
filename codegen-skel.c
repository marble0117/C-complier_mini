/* XCC (x86): Experimental C-subset Compiler.
  Copyright (c) 2002-2017, gondow@cs.titech.ac.jp, All rights reserved.
  $Id: codegen.c,v 1.1 2017/05/09 05:57:24 gondow Exp gondow $ */ 
/* ---------------------------------------------------------------------- */
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "xcc.h"
#include "misc.h"
#include "AST.h"
#include "type.h"
#include "symbol.h"
#include "codegen.h"

#define  LABEL_PREFIX  "L.XCC"
#define  TEXT_SECTION   "__TEXT,__text,regular,pure_instructions"
#define  DATA_SECTION   "__DATA,__data"
#define  RDATA_SECTION  "__TEXT,__cstring,cstring_literals"
static char *func_name;
static int   total_local_size;

static void emit_code (struct AST *ast, char *fmt, ...);
static void codegen_begin_block (struct AST *ast);
static void codegen_end_block (void);
static void codegen_begin_function (struct AST *ast);
static void codegen_end_function (void);

/* ---------------------------------------------------------------------- */

static void
emit_code (struct AST *ast, char *fmt, ...)
{
    va_list  argp;
    va_start (argp, fmt);
    vfprintf (xcc_out, fmt, argp);
    va_end   (argp);

    /* the argument 'ast' can be used for debug purpose */
}

#ifdef XCC_VIS
#include "vis/emit_code.h"
#endif

static void
codegen_begin_block (struct AST *ast)
{
    assert (!strcmp (ast->ast_type, "AST_compound_statement"));
    sym_table.local [++sym_table.local_index] = ast->u.local;
}

static void
codegen_end_block (void)
{
    sym_table.local_index--;
}

static void
codegen_begin_function (struct AST *ast)
{
    assert(!strcmp (ast->ast_type, "AST_function_definition"));
    sym_table.local_index = -1;
    sym_table.global = ast->u.func.global;
    sym_table.arg    = ast->u.func.arg;
    sym_table.label  = ast->u.func.label;
    sym_table.string = ast->u.func.string;
}

static void
codegen_end_function (void)
{
    /* do nothing */
}
// ここから上は（関数プロトタイプ宣言の追加等以外は）修正や拡張は不要のはず
/* ---------------------------------------------------------------------- */
// ここから下は好きに修正や拡張をしても構わない
void
codegen (void)
{
    // ここにあなたのプログラムを書く
}
/* ---------------------------------------------------------------------- */
