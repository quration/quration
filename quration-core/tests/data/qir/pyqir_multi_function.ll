; ModuleID = 'pyqir_multi_function'
source_filename = "pyqir_multi_function"

define void @main() #0 {
entry:
  call void @flip_and_measure(ptr null, ptr null)
  ret void
}

define internal void @flip_and_measure(ptr %0, ptr %1) {
entry:
  call void @__quantum__qis__x__body(ptr %0)
  call void @__quantum__qis__mz__body(ptr %0, ptr %1)
  ret void
}

declare void @__quantum__qis__x__body(ptr)

declare void @__quantum__qis__mz__body(ptr, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="1" "required_num_results"="1" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
