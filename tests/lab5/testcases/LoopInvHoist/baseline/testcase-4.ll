; ModuleID = 'cminus'
source_filename = "/home/haiqwa/2020fall-compiler_cminus/tests/lab5/./testcases/LoopInvHoist/testcase-8.cminus"

@a = global i32 zeroinitializer
declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define void @main() {
label_entry:
  store i32 2, i32* @a
  %op66 = load i32, i32* @a
  %op67 = load i32, i32* @a
  %op68 = mul i32 %op66, %op67
  %op69 = load i32, i32* @a
  %op70 = mul i32 %op68, %op69
  %op71 = load i32, i32* @a
  %op72 = mul i32 %op70, %op71
  %op73 = load i32, i32* @a
  %op74 = mul i32 %op72, %op73
  %op75 = load i32, i32* @a
  %op76 = mul i32 %op74, %op75
  %op77 = load i32, i32* @a
  %op78 = mul i32 %op76, %op77
  %op79 = load i32, i32* @a
  %op80 = mul i32 %op78, %op79
  %op81 = load i32, i32* @a
  %op82 = mul i32 %op80, %op81
  %op83 = load i32, i32* @a
  %op84 = mul i32 %op82, %op83
  %op85 = load i32, i32* @a
  %op86 = sdiv i32 %op84, %op85
  %op87 = load i32, i32* @a
  %op88 = sdiv i32 %op86, %op87
  %op89 = load i32, i32* @a
  %op90 = sdiv i32 %op88, %op89
  %op91 = load i32, i32* @a
  %op92 = sdiv i32 %op90, %op91
  %op93 = load i32, i32* @a
  %op94 = sdiv i32 %op92, %op93
  %op95 = load i32, i32* @a
  %op96 = sdiv i32 %op94, %op95
  %op97 = load i32, i32* @a
  %op98 = sdiv i32 %op96, %op97
  %op99 = load i32, i32* @a
  %op100 = sdiv i32 %op98, %op99
  %op101 = load i32, i32* @a
  %op102 = sdiv i32 %op100, %op101
  %op103 = load i32, i32* @a
  %op104 = sdiv i32 %op102, %op103
  %op57 = load i32, i32* @a
  %op58 = icmp sgt i32 %op57, 1
  %op59 = zext i1 %op58 to i32
  %op60 = icmp ne i32 %op59, 0
  br label %label7
label7:                                                ; preds = %label_entry, %label21
  %op107 = phi i32 [ %op114, %label21 ], [ undef, %label_entry ]
  %op108 = phi i32 [ %op115, %label21 ], [ undef, %label_entry ]
  %op109 = phi i32 [ %op116, %label21 ], [ undef, %label_entry ]
  %op110 = phi i32 [ %op117, %label21 ], [ undef, %label_entry ]
  %op111 = phi i32 [ %op118, %label21 ], [ undef, %label_entry ]
  %op112 = phi i32 [ 0, %label_entry ], [ %op23, %label21 ]
  %op113 = phi i32 [ %op119, %label21 ], [ undef, %label_entry ]
  %op9 = icmp slt i32 %op112, 1000000
  %op10 = zext i1 %op9 to i32
  %op11 = icmp ne i32 %op10, 0
  br i1 %op11, label %label12, label %label13
label12:                                                ; preds = %label7
  br label %label15
label13:                                                ; preds = %label7
  call void @output(i32 %op107)
  ret void
label15:                                                ; preds = %label12, %label30
  %op114 = phi i32 [ %op107, %label12 ], [ %op120, %label30 ]
  %op115 = phi i32 [ %op108, %label12 ], [ %op121, %label30 ]
  %op116 = phi i32 [ %op109, %label12 ], [ %op122, %label30 ]
  %op117 = phi i32 [ %op110, %label12 ], [ %op123, %label30 ]
  %op118 = phi i32 [ %op111, %label12 ], [ %op124, %label30 ]
  %op119 = phi i32 [ 0, %label12 ], [ %op32, %label30 ]
  %op17 = icmp slt i32 %op119, 2
  %op18 = zext i1 %op17 to i32
  %op19 = icmp ne i32 %op18, 0
  br i1 %op19, label %label20, label %label21
label20:                                                ; preds = %label15
  br label %label24
label21:                                                ; preds = %label15
  %op23 = add i32 %op112, 1
  br label %label7
label24:                                                ; preds = %label20, %label39
  %op120 = phi i32 [ %op114, %label20 ], [ %op125, %label39 ]
  %op121 = phi i32 [ %op115, %label20 ], [ %op126, %label39 ]
  %op122 = phi i32 [ %op116, %label20 ], [ %op127, %label39 ]
  %op123 = phi i32 [ %op117, %label20 ], [ %op128, %label39 ]
  %op124 = phi i32 [ 0, %label20 ], [ %op41, %label39 ]
  %op26 = icmp slt i32 %op124, 2
  %op27 = zext i1 %op26 to i32
  %op28 = icmp ne i32 %op27, 0
  br i1 %op28, label %label29, label %label30
label29:                                                ; preds = %label24
  br label %label33
label30:                                                ; preds = %label24
  %op32 = add i32 %op119, 1
  br label %label15
label33:                                                ; preds = %label29, %label48
  %op125 = phi i32 [ %op120, %label29 ], [ %op129, %label48 ]
  %op126 = phi i32 [ %op121, %label29 ], [ %op130, %label48 ]
  %op127 = phi i32 [ %op122, %label29 ], [ %op131, %label48 ]
  %op128 = phi i32 [ 0, %label29 ], [ %op50, %label48 ]
  %op35 = icmp slt i32 %op128, 2
  %op36 = zext i1 %op35 to i32
  %op37 = icmp ne i32 %op36, 0
  br i1 %op37, label %label38, label %label39
label38:                                                ; preds = %label33
  br label %label42
label39:                                                ; preds = %label33
  %op41 = add i32 %op124, 1
  br label %label24
label42:                                                ; preds = %label38, %label61
  %op129 = phi i32 [ %op125, %label38 ], [ %op132, %label61 ]
  %op130 = phi i32 [ %op126, %label38 ], [ %op133, %label61 ]
  %op131 = phi i32 [ 0, %label38 ], [ %op63, %label61 ]
  %op44 = icmp slt i32 %op131, 2
  %op45 = zext i1 %op44 to i32
  %op46 = icmp ne i32 %op45, 0
  br i1 %op46, label %label47, label %label48
label47:                                                ; preds = %label42
  br label %label51
label48:                                                ; preds = %label42
  %op50 = add i32 %op128, 1
  br label %label33
label51:                                                ; preds = %label47, %label65
  %op132 = phi i32 [ %op129, %label47 ], [ %op104, %label65 ]
  %op133 = phi i32 [ 0, %label47 ], [ %op106, %label65 ]
  %op53 = icmp slt i32 %op133, 2
  %op54 = zext i1 %op53 to i32
  %op55 = icmp ne i32 %op54, 0
  br i1 %op55, label %label56, label %label61
label56:                                                ; preds = %label51
  br i1 %op60, label %label64, label %label65
label61:                                                ; preds = %label51
  %op63 = add i32 %op131, 1
  br label %label42
label64:                                                ; preds = %label56
  store i32 1, i32* @a
  br label %label65
label65:                                                ; preds = %label56, %label64
  %op106 = add i32 %op133, 1
  br label %label51
}
