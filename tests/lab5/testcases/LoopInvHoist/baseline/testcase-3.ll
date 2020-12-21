; ModuleID = 'cminus'
source_filename = "/home/haiqwa/2020fall-compiler_cminus/tests/lab5/./testcases/LoopInvHoist/testcase-7.cminus"

@a = global i32 zeroinitializer
declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define void @main() {
label_entry:
  store i32 2, i32* @a
  %op57 = load i32, i32* @a
  %op58 = load i32, i32* @a
  %op59 = mul i32 %op57, %op58
  %op60 = load i32, i32* @a
  %op61 = mul i32 %op59, %op60
  %op62 = load i32, i32* @a
  %op63 = mul i32 %op61, %op62
  %op64 = load i32, i32* @a
  %op65 = mul i32 %op63, %op64
  %op66 = load i32, i32* @a
  %op67 = mul i32 %op65, %op66
  %op68 = load i32, i32* @a
  %op69 = mul i32 %op67, %op68
  %op70 = load i32, i32* @a
  %op71 = mul i32 %op69, %op70
  %op72 = load i32, i32* @a
  %op73 = mul i32 %op71, %op72
  %op74 = load i32, i32* @a
  %op75 = mul i32 %op73, %op74
  %op76 = load i32, i32* @a
  %op77 = sdiv i32 %op75, %op76
  %op78 = load i32, i32* @a
  %op79 = sdiv i32 %op77, %op78
  %op80 = load i32, i32* @a
  %op81 = sdiv i32 %op79, %op80
  %op82 = load i32, i32* @a
  %op83 = sdiv i32 %op81, %op82
  %op84 = load i32, i32* @a
  %op85 = sdiv i32 %op83, %op84
  %op86 = load i32, i32* @a
  %op87 = sdiv i32 %op85, %op86
  %op88 = load i32, i32* @a
  %op89 = sdiv i32 %op87, %op88
  %op90 = load i32, i32* @a
  %op91 = sdiv i32 %op89, %op90
  %op92 = load i32, i32* @a
  %op93 = sdiv i32 %op91, %op92
  %op94 = load i32, i32* @a
  %op95 = sdiv i32 %op93, %op94
  br label %label7
label7:                                                ; preds = %label_entry, %label21
  %op101 = phi i32 [ %op108, %label21 ], [ undef, %label_entry ]
  %op102 = phi i32 [ %op109, %label21 ], [ undef, %label_entry ]
  %op103 = phi i32 [ %op110, %label21 ], [ undef, %label_entry ]
  %op104 = phi i32 [ %op111, %label21 ], [ undef, %label_entry ]
  %op105 = phi i32 [ %op112, %label21 ], [ undef, %label_entry ]
  %op106 = phi i32 [ 0, %label_entry ], [ %op23, %label21 ]
  %op107 = phi i32 [ %op113, %label21 ], [ undef, %label_entry ]
  %op9 = icmp slt i32 %op106, 1000000
  %op10 = zext i1 %op9 to i32
  %op11 = icmp ne i32 %op10, 0
  br i1 %op11, label %label12, label %label13
label12:                                                ; preds = %label7
  br label %label15
label13:                                                ; preds = %label7
  call void @output(i32 %op101)
  ret void
label15:                                                ; preds = %label12, %label30
  %op108 = phi i32 [ %op101, %label12 ], [ %op114, %label30 ]
  %op109 = phi i32 [ %op102, %label12 ], [ %op115, %label30 ]
  %op110 = phi i32 [ %op103, %label12 ], [ %op116, %label30 ]
  %op111 = phi i32 [ %op104, %label12 ], [ %op117, %label30 ]
  %op112 = phi i32 [ %op105, %label12 ], [ %op118, %label30 ]
  %op113 = phi i32 [ 0, %label12 ], [ %op32, %label30 ]
  %op17 = icmp slt i32 %op113, 2
  %op18 = zext i1 %op17 to i32
  %op19 = icmp ne i32 %op18, 0
  br i1 %op19, label %label20, label %label21
label20:                                                ; preds = %label15
  br label %label24
label21:                                                ; preds = %label15
  %op23 = add i32 %op106, 1
  br label %label7
label24:                                                ; preds = %label20, %label39
  %op114 = phi i32 [ %op108, %label20 ], [ %op119, %label39 ]
  %op115 = phi i32 [ %op109, %label20 ], [ %op120, %label39 ]
  %op116 = phi i32 [ %op110, %label20 ], [ %op121, %label39 ]
  %op117 = phi i32 [ %op111, %label20 ], [ %op122, %label39 ]
  %op118 = phi i32 [ 0, %label20 ], [ %op41, %label39 ]
  %op26 = icmp slt i32 %op118, 2
  %op27 = zext i1 %op26 to i32
  %op28 = icmp ne i32 %op27, 0
  br i1 %op28, label %label29, label %label30
label29:                                                ; preds = %label24
  br label %label33
label30:                                                ; preds = %label24
  %op32 = add i32 %op113, 1
  br label %label15
label33:                                                ; preds = %label29, %label48
  %op119 = phi i32 [ %op114, %label29 ], [ %op123, %label48 ]
  %op120 = phi i32 [ %op115, %label29 ], [ %op124, %label48 ]
  %op121 = phi i32 [ %op116, %label29 ], [ %op125, %label48 ]
  %op122 = phi i32 [ 0, %label29 ], [ %op50, %label48 ]
  %op35 = icmp slt i32 %op122, 2
  %op36 = zext i1 %op35 to i32
  %op37 = icmp ne i32 %op36, 0
  br i1 %op37, label %label38, label %label39
label38:                                                ; preds = %label33
  br label %label42
label39:                                                ; preds = %label33
  %op41 = add i32 %op118, 1
  br label %label24
label42:                                                ; preds = %label38, %label98
  %op123 = phi i32 [ %op119, %label38 ], [ %op126, %label98 ]
  %op124 = phi i32 [ %op120, %label38 ], [ %op127, %label98 ]
  %op125 = phi i32 [ 0, %label38 ], [ %op100, %label98 ]
  %op44 = icmp slt i32 %op125, 2
  %op45 = zext i1 %op44 to i32
  %op46 = icmp ne i32 %op45, 0
  br i1 %op46, label %label47, label %label48
label47:                                                ; preds = %label42
  br label %label51
label48:                                                ; preds = %label42
  %op50 = add i32 %op122, 1
  br label %label33
label51:                                                ; preds = %label47, %label56
  %op126 = phi i32 [ %op123, %label47 ], [ %op95, %label56 ]
  %op127 = phi i32 [ 0, %label47 ], [ %op97, %label56 ]
  %op53 = icmp slt i32 %op127, 2
  %op54 = zext i1 %op53 to i32
  %op55 = icmp ne i32 %op54, 0
  br i1 %op55, label %label56, label %label98
label56:                                                ; preds = %label51
  %op97 = add i32 %op127, 1
  br label %label51
label98:                                                ; preds = %label51
  %op100 = add i32 %op125, 1
  br label %label42
}
