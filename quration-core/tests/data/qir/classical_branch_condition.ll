; ModuleID = 'classical_branch_condition'
source_filename = "classical_branch_condition"

@0 = internal constant [3 x i8] c"t3\00"
@1 = internal constant [3 x i8] c"r0\00"
@2 = internal constant [3 x i8] c"r1\00"
@3 = internal constant [3 x i8] c"r2\00"

define void @main() #0 {
entry:
  call void @__quantum__qis__x__body(ptr null)
  call void @__quantum__qis__mz__body(ptr null, ptr null)
  %0 = call i1 @__quantum__rt__read_result(ptr null)
  %1 = zext i1 %0 to i64
  %2 = icmp sgt i64 %1, 0
  br i1 %2, label %then, label %else

then:                                             ; preds = %entry
  call void @__quantum__qis__x__body(ptr inttoptr (i64 1 to ptr))
  br label %join

else:                                             ; preds = %entry
  call void @__quantum__qis__x__body(ptr inttoptr (i64 2 to ptr))
  br label %join

join:                                             ; preds = %else, %then
  call void @__quantum__qis__mz__body(ptr inttoptr (i64 1 to ptr), ptr inttoptr (i64 1 to ptr))
  call void @__quantum__qis__mz__body(ptr inttoptr (i64 2 to ptr), ptr inttoptr (i64 2 to ptr))
  call void @__quantum__rt__tuple_record_output(i64 3, ptr @0)
  call void @__quantum__rt__result_record_output(ptr null, ptr @1)
  call void @__quantum__rt__result_record_output(ptr inttoptr (i64 1 to ptr), ptr @2)
  call void @__quantum__rt__result_record_output(ptr inttoptr (i64 2 to ptr), ptr @3)
  ret void
}

declare void @__quantum__qis__x__body(ptr)

declare void @__quantum__qis__mz__body(ptr, ptr)

declare i1 @__quantum__rt__read_result(ptr)

declare void @__quantum__rt__tuple_record_output(i64, ptr)

declare void @__quantum__rt__result_record_output(ptr, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="3" "required_num_results"="3" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
