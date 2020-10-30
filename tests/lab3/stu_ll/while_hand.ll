; ModuleID = 'fun_hand.c'
source_filename = "fun_hand.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"
; end of header

; === Begin of function main() ===
define i32 @main() #0 {
    %a = alloca i32
    %i = alloca i32
    store i32 10, i32* %a
    store i32 0, i32* %i
    br label %while_statement_cmp
while_statement_cmp:
    ; while结构 判断块
    ; 取出i的数值
    %i_val = load i32, i32* %i
    ; 将i与10比较，结果存入res
    %res = icmp slt i32 %i_val, 10
    ; 若小于则执行while主语句块，否则跳转至while代码块后的代码
    br i1 %res, label %while_statement_main, label %finally
while_statement_main:
    ; while结构 代码块
    ; 执行此处时必定由while_statement_cmp跳转，此时%i_val存储着i的最新的值，故不重复取值
    %i_val_1 = add i32 %i_val, 1
    ; 更新i的值 i = i + 1
    store i32 %i_val_1, i32* %i
    ; a = a + i
    %a_val = load i32, i32* %a
    %a_val_1 = add i32 %a_val, %i_val_1
    store i32 %a_val_1, i32* %a
    ; 无条件跳转至判断块
    br label %while_statement_cmp
finally:
    ; return a
    %ret_val = load i32, i32* %a
    ret i32 %ret_val

}
; === End of function main() ===