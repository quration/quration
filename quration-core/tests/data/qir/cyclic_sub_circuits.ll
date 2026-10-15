; ModuleID = 'cyclic_sub_circuits'
source_filename = "cyclic_sub_circuits"

define void @main() #0 {
entry:
  call void @sub_a(ptr null)
  ret void
}

define internal void @sub_a(ptr %0) {
entry:
  call void @sub_b(ptr %0)
  ret void
}

define internal void @sub_b(ptr %0) {
entry:
  call void @sub_a(ptr %0)
  ret void
}

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="1" "required_num_results"="0" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
