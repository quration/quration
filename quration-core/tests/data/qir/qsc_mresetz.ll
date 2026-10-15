; ModuleID = 'qsc_mresetz'
source_filename = "qsc_mresetz"

define void @main() #0 {
entry:
  call void @__quantum__qis__x__body(ptr null)
  call void @__quantum__qis__mresetz__body(ptr null, ptr null)
  %0 = call i1 @__quantum__qis__read_result__body(ptr null)
  br i1 %0, label %then, label %cont

then:                                             ; preds = %entry
  call void @__quantum__qis__x__body(ptr inttoptr (i64 1 to ptr))
  br label %cont

cont:                                             ; preds = %then, %entry
  call void @__quantum__qis__mz__body(ptr null, ptr inttoptr (i64 1 to ptr))
  call void @__quantum__qis__mz__body(ptr inttoptr (i64 1 to ptr), ptr inttoptr (i64 2 to ptr))
  ret void
}

declare void @__quantum__qis__x__body(ptr)

declare void @__quantum__qis__mresetz__body(ptr, ptr)

declare i1 @__quantum__qis__read_result__body(ptr)

declare void @__quantum__qis__mz__body(ptr, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="2" "required_num_results"="3" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
