; ModuleID = 'fun_hand.c'
source_filename = "fun_hand.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"
; end of header

; === Begin of funcion callee() ===
; 接受一个传入值a
define i32 @callee(i32 %a) #0 {
    %ret_val = mul i32 2, %a
    ret i32 %ret_val
}
; === End of function callee() ===


; === Begin of function main() ===
define i32 @main() #0 {
    ; 调用函数callee并存储返回值
    %ret_val = call i32 @callee(i32 110)
    ret i32 %ret_val
}
; === End of function main() ===