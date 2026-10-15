; ModuleID = 'duplicate_record_output'
source_filename = "duplicate_record_output"

@0 = internal constant [3 x i8] c"t2\00"
@1 = internal constant [3 x i8] c"r0\00"
@2 = internal constant [3 x i8] c"r1\00"

define void @main() #0 {
entry:
  call void @__quantum__qis__mz__body(ptr null, ptr null)
  call void @__quantum__rt__tuple_record_output(i64 2, ptr @0)
  call void @__quantum__rt__result_record_output(ptr null, ptr @1)
  call void @__quantum__rt__result_record_output(ptr null, ptr @2)
  ret void
}

declare void @__quantum__qis__mz__body(ptr, ptr)

declare void @__quantum__rt__tuple_record_output(i64, ptr)

declare void @__quantum__rt__result_record_output(ptr, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="1" "required_num_results"="1" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
