; ModuleID = 'fun_hand.c'
source_filename = "fun_hand.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"
; end of header

; === Begin of function main() ===
define i32 @main() #0 {
    ; 为a分配空间
    %a = alloca float, align 4
    ; 将5.555存入a，16进制float表示来自clang
    store float 0x40163851E0000000, float* %a
    ; 取出float的数值
    %a_val = load float, float* %a
    ; 比较两数大小，结果存储置if_cond
    %if_cond = fcmp ugt float %a_val, 1.0
    ; 条件跳转
    br i1 %if_cond, label %if_structure, label %finally
if_structure:   ; if 条件成立
    ret i32 233
finally:    ; if代码块后的结果
    ret i32 0
}
; === End of function main() ===