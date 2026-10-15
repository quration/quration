; ModuleID = 'barrier'
source_filename = "barrier"

define void @main() #0 {
entry:
  call void @__quantum__qis__x__body(ptr null)
  call void @__quantum__qis__barrier__body()
  call void @__quantum__qis__mz__body(ptr null, ptr null)
  ret void
}

declare void @__quantum__qis__x__body(ptr)

declare void @__quantum__qis__barrier__body()

declare void @__quantum__qis__mz__body(ptr, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="1" "required_num_results"="1" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
