; ModuleID = 'rzz_rxx_ryy'
source_filename = "rzz_rxx_ryy"

define void @main() #0 {
entry:
  call void @__quantum__qis__rzz__body(double 1.000000e+00, ptr null, ptr inttoptr (i64 1 to ptr))
  call void @__quantum__qis__rxx__body(double 2.000000e+00, ptr null, ptr inttoptr (i64 1 to ptr))
  call void @__quantum__qis__ryy__body(double 3.000000e+00, ptr null, ptr inttoptr (i64 1 to ptr))
  ret void
}

declare void @__quantum__qis__rzz__body(double, ptr, ptr)

declare void @__quantum__qis__rxx__body(double, ptr, ptr)

declare void @__quantum__qis__ryy__body(double, ptr, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="2" "required_num_results"="0" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
