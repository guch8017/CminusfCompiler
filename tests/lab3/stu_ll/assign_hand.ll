; ModuleID = 'assign.c'
source_filename = "assign.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"
; end of header

; Funtion Attrs: noinline
; === Begin of funcion main() ===
define i32 @main() #0 {
    %a = alloca [10 x i32]
    ; 计算需要用到的偏移值
    %off_0 = getelementptr [10 x i32], [10 x i32]* %a, i32 0, i32 0
    %off_1 = getelementptr [10 x i32], [10 x i32]* %a, i32 0, i32 1
    ; 将10存入a[0]
    store i32 10, i32* %off_0
    ; 访问a[0]取出数字
    %tmp_a0 = load i32, i32* %off_0
    ; 计算a[0] * 2
    %tmp_mul = mul i32 %tmp_a0, 2
    ; 存入a[1]
    store i32 %tmp_mul, i32* %off_1
    ; 取出a[1]
    %tmp_a1 = load i32, i32* %off_1
    ; 返回结果
    ret i32 %tmp_a1
}
; === End of function main() ===