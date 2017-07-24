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

static int label_count = 0;
static int exp_id_mem = 0;  //右辺値:

static void emit_code (struct AST *ast, char *fmt, ...);
static void codegen_begin_block (struct AST *ast);
static void codegen_end_block (void);
static void codegen_begin_function (struct AST *ast);
static void codegen_end_function (void);

static void codegen_exp_id (struct AST *ast);
static void codegen_exp_funcall (struct AST *ast);
static void codegen_exp (struct AST *ast);
static void codegen_stmt (struct AST *ast_stmt);
static void codegen_block (struct AST *ast_block);
static void codegen_func (struct AST *ast);
static void codegen_dec (struct AST *ast);

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
static void
codegen_exp_id (struct AST *ast)
{
    int offset;
    struct Symbol *sym = sym_lookup (ast->child [0]->u.id);
    assert (sym != NULL);

    switch (sym->name_space) {
    case NS_LOCAL:
    case NS_ARG:
        if (sym->name_space == NS_LOCAL)
            offset = - (sym->offset + 4);
        else if (sym->name_space == NS_ARG)
            offset =  sym->offset + 8;
        else
            assert (0);

        // char型には非対応
        emit_code (ast, "\tpushl   %d(%%ebp) \t# %s, %d\n",
                   offset, sym->name, sym->offset);
	break;
    case NS_GLOBAL:
        // char型には非対応
        if (sym->type->kind == TYPE_KIND_FUNCTION || exp_id_mem) {
            emit_code (ast, "\tpushl   $_%s\n", sym->name);
        } else {
            emit_code (ast, "\tpushl   _%s\n", sym->name);
        }
	break;
    case NS_LABEL: /* falling through */
    default: assert (0); break;
    }
}

static void
codegen_exp_funcall (struct AST *ast_func)
{
    int args_size = 0;
    struct AST *ast, *ast_exp;

    assert (!strcmp (ast_func->ast_type, "AST_expression_funcall1")
	    || !strcmp (ast_func->ast_type, "AST_expression_funcall2"));

    /* push arguments in reverse order (funcall2 has no arguments) */
    if (!strcmp (ast_func->ast_type, "AST_expression_funcall1")) {
        /* for Mac 16-bytes alignment */
        emit_code (ast_func, "\tsubl    $%d, %%esp\n",
                   STACK_ALIGNMENT - (ast_func->child [1]->u.arg_size % STACK_ALIGNMENT));

	for (ast = ast_func->child [1]; ; ast = ast->child [0]) {
	    if (!strcmp (ast->ast_type,
                         "AST_argument_expression_list_single")) {
		ast_exp = ast->child [0];
            } else if (!strcmp (ast->ast_type,
                                "AST_argument_expression_list_pair")) {
		ast_exp = ast->child [1];
            } else {
                assert (0);
	    }
            args_size += ROUNDUP_INT (ast_exp->type->size);
	    codegen_exp (ast_exp);
	    if (!strcmp (ast->ast_type,
                         "AST_argument_expression_list_single"))
                break;
	}
    }

    codegen_exp (ast_func->child [0]);
    emit_code (ast_func, "\tpopl    %%eax\n");
    emit_code (ast_func, "\tcall    *%%eax\n");
    emit_code (ast_func, "\taddl    $%d, %%esp \t# pop args\n",
               ROUNDUP_STACK(args_size));
    emit_code (ast_func, "\tpushl   %%eax\n");
}

static void
codegen_exp (struct AST *ast)
{
    if (!strcmp (ast->ast_type, "AST_expression_int")) {
	emit_code (ast, "\tpushl   $%d\n", ast->u.int_val);
    } else if (!strcmp (ast->ast_type, "AST_expression_string")) {
        struct String *string = string_lookup (ast->u.id);
        assert (string != NULL);
	    emit_code (ast, "\tpushl   $%s.%s \t# \"%s\"\n",
                   LABEL_PREFIX, string->label, string->data);
    } else if (!strcmp (ast->ast_type, "AST_expression_id")) {
	    codegen_exp_id (ast);
    } else if (   !strcmp (ast->ast_type, "AST_expression_funcall1")
               || !strcmp (ast->ast_type, "AST_expression_funcall2")) {
	    codegen_exp_funcall (ast);
/*
    } else if (.....) {  // 他の expression の場合のコードをここに追加する
 */
    } else if (!strcmp (ast->ast_type, "AST_expression_assign")) {
        // a = b
        exp_id_mem = 0;
        codegen_exp (ast->child[1]);    //right
        exp_id_mem = 1;
        codegen_exp (ast->child[0]);    //left
        exp_id_mem = 0;
        emit_code (ast, "\tpopl    %%eax\n");  // %eax <- a
        emit_code (ast, "\tmovl    0(%%esp), %%ecx\n");  // %ecx <- b
        emit_code (ast, "\tmovl    %%ecx, 0(%%eax)\n");  // a <- b
    } else if (!strcmp (ast->ast_type, "AST_expression_add")) {
        codegen_exp (ast->child[0]); //push a
        codegen_exp (ast->child[1]); //push b
        emit_code (ast, "\tpopl    %%ecx\n");
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\taddl    %%ecx, %%eax\n");
        emit_code (ast, "\tpushl   %%eax\n");
    } else if (!strcmp (ast->ast_type, "AST_expression_sub")) {
        codegen_exp (ast->child[0]); //push a
        codegen_exp (ast->child[1]); //push b
        emit_code (ast, "\tpopl    %%ecx\n");
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\tsubl    %%ecx, %%eax\n");
        emit_code (ast, "\tpushl   %%eax\n");
    } else if (!strcmp (ast->ast_type, "AST_expression_mul")) {
        codegen_exp (ast->child[0]); //push a
        codegen_exp (ast->child[1]); //push b
        emit_code (ast, "\tpopl    %%ecx\n");
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\timull   %%ecx, %%eax\n");
        emit_code (ast, "\tpushl   %%eax\n");
    } else if (!strcmp (ast->ast_type, "AST_expression_div")) {
        codegen_exp (ast->child[0]); //push a
        codegen_exp (ast->child[1]); //push b
        emit_code (ast, "\tpopl    %%ecx\n");
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\tcltd\n");
        emit_code (ast, "\tidivl    %%ecx\n");
        emit_code (ast, "\tpushl   %%eax\n");
    } else if (!strcmp (ast->ast_type, "AST_expression_less")) {
        codegen_exp (ast->child[0]);
        codegen_exp (ast->child[1]);
        emit_code (ast, "\tpopl    %%ecx\n");
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\tcmpl    %%ecx, %%eax\n");
        emit_code (ast, "\tsetl    %%al\n");
        emit_code (ast, "\tmovzbl  %%al, %%eax\n");
        emit_code (ast, "\tpushl   %%eax\n");
    } else if (!strcmp (ast->ast_type, "AST_expression_eq")) {
        codegen_exp (ast->child[0]);
        codegen_exp (ast->child[1]);
        emit_code (ast, "\tpopl    %%ecx\n");
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\tcmpl    %%ecx, %%eax\n");
        emit_code (ast, "\tsete    %%al\n");
        emit_code (ast, "\tmovzbl  %%al, %%eax\n");
        emit_code (ast, "\tpushl   %%eax\n");
    } else if (!strcmp (ast->ast_type, "AST_expression_lor")) {
        codegen_exp (ast->child[0]);     //expression1
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\tcmpl    $0, %%eax\n");
        emit_code (ast, "\tjne     L%d\n", label_count);    //if 1(true) jmp true
        codegen_exp (ast->child[1]);     //expression2
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\tcmpl    $0, %%eax\n");
        emit_code (ast, "\tjne     L%d\n", label_count);    //if 1(true) jmp true
        emit_code (ast, "\tjmp     L%d\n", label_count+1);  //else jmp false
        emit_code (ast, "L%d:\n", label_count++);  //true
        emit_code (ast, "\tpushl   $1\n");
        emit_code (ast, "\tjmp     L%d\n", label_count+1);
        emit_code (ast, "L%d:\n", label_count++);  //false
        emit_code (ast, "\tpushl   $0\n");
        emit_code (ast, "L%d:\n", label_count++);
    } else if (!strcmp (ast->ast_type, "AST_expression_land")) {
        codegen_exp (ast->child[0]);     //expression1
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\tcmpl    $0, %%eax\n");
        emit_code (ast, "\tje      L%d\n", label_count);    //if 0(false) jmp false
        codegen_exp (ast->child[1]);     //expression2
        emit_code (ast, "\tpopl    %%eax\n");
        emit_code (ast, "\tcmpl    $0, %%eax\n");
        emit_code (ast, "\tje      L%d\n", label_count);    //if 0(false) jmp true
        emit_code (ast, "\tjmp     L%d\n", label_count+1);  //else jmp true
        emit_code (ast, "L%d:\n", label_count++);  //false
        emit_code (ast, "\tpushl   $0\n");
        emit_code (ast, "\tjmp     L%d\n", label_count+1);
        emit_code (ast, "L%d:\n", label_count++);  //true
        emit_code (ast, "\tpushl   $1\n");
        emit_code (ast, "L%d:\n", label_count++);
    } else {
        fprintf (stderr, "ast_type: %s\n", ast->ast_type);
        assert (0);
    }
}

static void
codegen_stmt (struct AST *ast_stmt)
{
    if (!strcmp (ast_stmt->ast_type, "AST_statement_exp")) {
	if (!strcmp (ast_stmt->child [0]->ast_type, "AST_expression_opt_single")) {
	    codegen_exp (ast_stmt->child [0]->child [0]);
            emit_code (ast_stmt, "\taddl    $4, %%esp\n");
	} else if (!strcmp (ast_stmt->child [0]->ast_type, "AST_expression_opt_null")) {
            /* nothing to do */
        } else {
            assert (0);
        }
    } else if (!strcmp (ast_stmt->ast_type, "AST_statement_comp")) {
	codegen_block (ast_stmt->child [0]);
/*
    } else if (.....) {  // 他の statement の場合のコードをここに追加する
 */
    } else if (!strcmp (ast_stmt->ast_type, "AST_statement_if")) {
        codegen_exp (ast_stmt->child[0]);     //expression
        emit_code (ast_stmt, "\tcmpl    $0, %%eax\n");
        emit_code (ast_stmt, "\tje      L%d\n", label_count);
        codegen_stmt(ast_stmt->child[1]);     //statement
        emit_code (ast_stmt, "L%d:\n", label_count++);
    } else if (!strcmp (ast_stmt->ast_type, "AST_statement_ifelse")) {
        codegen_exp (ast_stmt->child[0]);     //expression
        emit_code (ast_stmt, "\tcmpl    $0, %%eax\n");
        emit_code (ast_stmt, "\tje      L%d\n", label_count);
        codegen_stmt (ast_stmt->child[1]);     //statement_1
        emit_code (ast_stmt, "\tjmp     L%d\n", label_count+1);
        emit_code (ast_stmt, "L%d:\n", label_count++);
        codegen_stmt (ast_stmt->child[2]);     //statement_2
        emit_code (ast_stmt, "L%d:\n", label_count);
    } else if (!strcmp (ast_stmt->ast_type, "AST_statement_while")) {
        emit_code (ast_stmt, "L%d:\n", label_count);
        codegen_exp (ast_stmt->child[0]);      //expression
        emit_code (ast_stmt, "\tcmpl    $0, %%eax\n");
        emit_code (ast_stmt, "\tje      L%d\n", (label_count+1));
        codegen_stmt (ast_stmt->child[1]);     //statement
        emit_code (ast_stmt, "\tjmp     L%d\n", label_count++);
        emit_code (ast_stmt, "L%d:\n", label_count++);
    } else if (!strcmp (ast_stmt->ast_type, "AST_statement_goto")) {
    } else if (!strcmp (ast_stmt->ast_type, "AST_statement_return")) {
        emit_code (ast_stmt, "\tpopl    %%eax\n");
        emit_code (ast_stmt, "\tjmp     %s.RE.%s\n", LABEL_PREFIX,func_name);
    } else {
        assert (0);
    }
}

static void
codegen_func (struct AST *ast)
{
    struct String *string, *head;

    assert (!strcmp (ast->ast_type, "AST_function_definition"));

    codegen_begin_function (ast);

    /* string literals */
    head = sym_table.string;
    if (head != NULL) {
	emit_code (ast, "\t.section %s\n", RDATA_SECTION);
        for (string = head; string != NULL; string = string->next) {
            emit_code (ast, "%s.%s:\n", LABEL_PREFIX, string->label);
            emit_code (ast, "\t.ascii  \"%s\\0\"\n", string->data);
        }
    }

    func_name = ast->type->id;
    total_local_size = ast->u.func.total_local_size;

    emit_code (ast, "\t.section %s\n", TEXT_SECTION);
    emit_code (ast, "\t.globl  _%s\n", func_name);
    emit_code (ast, "\t.align 4, 0x90\n");
    emit_code (ast, "_%s:\n", func_name);
    emit_code (ast, "\tpushl   %%ebp\n");
    emit_code (ast, "\tmovl    %%esp, %%ebp\n");
    emit_code (ast, "\tsubl    $8, %%esp\n"); /* for Mac 16-bytes alignment */
    /* allocate space for local variables */
    emit_code (ast, "\tsubl    $%d, %%esp\n", total_local_size);

    /* function body */
    codegen_block (ast->child [2]);

    /* function epilogue */
    emit_code (ast, "%s.RE.%s:\n", LABEL_PREFIX, func_name);
    emit_code (ast, "\tmovl    %%ebp, %%esp\n");
    emit_code (ast, "\tpopl    %%ebp\n");
    emit_code (ast, "\tret\n");

    codegen_end_function ();
}

static void
codegen_dec (struct AST *ast)
{
    assert (!strcmp (ast->ast_type, "AST_declaration"));
    if (ast->type->size <= 0)
	return;

    emit_code (ast, "\t.globl  _%s\n", ast->type->id);
    emit_code (ast, "\t.section %s\n", DATA_SECTION);
    /* 1 byte alignment for char, and 4 byte alighment for int */
    if (ast->type->size == 4) {
        emit_code (ast, "\t.align  2\n");
    }
    emit_code (ast, "_%s:\n", ast->type->id);
    emit_code (ast, "\t.skip   %d\n\n", ast->type->size);
}

static void
codegen_block (struct AST *ast_block)
{
    struct AST *ast, *ast_stmt_list;
    assert (!strcmp (ast_block->ast_type, "AST_compound_statement"));
    codegen_begin_block (ast_block);
    
    ast_stmt_list = ast_block->child [1];
    ast = search_AST_bottom (ast_stmt_list, "AST_statement_list_single", NULL);
    while (1) {
        if (!strcmp (ast->ast_type, "AST_statement_list_single"))
            codegen_stmt (ast->child [0]); 
        else if (!strcmp (ast->ast_type, "AST_statement_list_pair"))
            codegen_stmt (ast->child [1]);
        else
            assert (0);
	if (ast == ast_stmt_list)
	    break;
	ast = ast->parent;
    } 
    codegen_end_block ();
}
/* ---------------------------------------------------------------------- */
void
codegen (void)
{
    struct AST *ast, *ast_ext;
    ast = search_AST_bottom (ast_root, "AST_translation_unit_single", NULL);

    while (1) {
        if (!strcmp (ast->ast_type, "AST_translation_unit_single"))
            ast_ext = ast->child [0];
        else if (!strcmp (ast->ast_type, "AST_translation_unit_pair"))
            ast_ext = ast->child [1];
        else
            assert (0);

        if (!strcmp (ast_ext->ast_type, "AST_external_declaration_func"))
	       codegen_func (ast_ext->child [0]);
        else if (!strcmp (ast_ext->ast_type, "AST_external_declaration_dec"))
	       codegen_dec (ast_ext->child [0]);
        else 
            assert (0);

	if (ast == ast_root)
	    break;
	ast = ast->parent;
    }
}
/* ---------------------------------------------------------------------- */
