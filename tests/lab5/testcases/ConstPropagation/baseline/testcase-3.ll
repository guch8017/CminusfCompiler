declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define i32 @meanless(i32 %arg0) {
label_entry:
  br label %label4
label4:                                                ; preds = %label_entry, %label10
  %op21 = phi i32 [ 178, %label10 ], [ undef, %label_entry ]
  %op22 = phi i32 [ 0, %label_entry ], [ %op18, %label10 ]
  %op7 = icmp slt i32 %op22, %arg0
  %op8 = zext i1 %op7 to i32
  %op9 = icmp ne i32 %op8, 0
  br i1 %op9, label %label10, label %label19
label10:                                                ; preds = %label4
  %op18 = add i32 %op22, 1
  br label %label4
label19:                                                ; preds = %label4
  ret i32 %op21
}
define void @main() {
label_entry:
  br label %label5
label5:                                                ; preds = %label_entry, %label17
  %op20 = phi i32 [ 0, %label_entry ], [ %op19, %label17 ]
  %op7 = icmp slt i32 %op20, 10000000
  %op8 = zext i1 %op7 to i32
  %op9 = icmp ne i32 %op8, 0
  br i1 %op9, label %label10, label %label13
label10:                                                ; preds = %label5
  br label %label14
label13:                                                ; preds = %label5
  ret void
label14:                                                ; preds = %label10
  %op16 = call i32 @meanless(i32 21)
  br label %label17
label17:                                                ; preds = %label14
  %op19 = add i32 %op20, 1
  br label %label5
}
