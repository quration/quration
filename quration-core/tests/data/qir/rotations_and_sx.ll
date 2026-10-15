; ModuleID = 'rotations_and_sx'
source_filename = "rotations_and_sx"

define void @main() #0 {
entry:
  call void @__quantum__qis__rx__body(double 5.000000e-01, ptr null)
  call void @__quantum__qis__ry__body(double 1.250000e+00, ptr null)
  call void @__quantum__qis__rz__body(double 2.000000e+00, ptr null)
  call void @__quantum__qis__sx__body(ptr null)
  call void @__quantum__qis__mz__body(ptr null, ptr null)
  ret void
}

declare void @__quantum__qis__rx__body(double, ptr)

declare void @__quantum__qis__ry__body(double, ptr)

declare void @__quantum__qis__rz__body(double, ptr)

declare void @__quantum__qis__sx__body(ptr)

declare void @__quantum__qis__mz__body(ptr, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="1" "required_num_results"="1" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
