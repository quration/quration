; Hand-written: a phi at a switch join. Neither pyqir nor QDK can emit a QIR `switch`, and a phi is
; a construct Quration's importer does not support, so this fixture cannot be produced by the
; generator (quration-core/scripts/generate-qir-test-data). It is otherwise complete and valid QIR
; (all functions declared, entry point and module flags present) so the importer rejects it for the
; phi node specifically, not for any incidental malformation.
@0 = internal constant [4 x i8] c"0_i\00"

define i64 @main() #0 {
entry:
  call void @__quantum__rt__initialize(ptr null)
  call void @__quantum__qis__mz__body(ptr null, ptr writeonly null)
  %c0 = call i1 @__quantum__rt__read_result(ptr null)
  %v0 = zext i1 %c0 to i64
  switch i64 %v0, label %default [
    i64 0, label %case0
  ]
case0:
  br label %join
default:
  br label %join
join:
  %v1 = phi i64 [1, %case0], [2, %default]
  call void @__quantum__rt__int_record_output(i64 %v1, ptr @0)
  ret i64 0
}
declare void @__quantum__rt__initialize(ptr)
declare void @__quantum__qis__mz__body(ptr, ptr writeonly) #1
declare i1 @__quantum__rt__read_result(ptr readonly)
declare void @__quantum__rt__int_record_output(i64, ptr)
attributes #0 = { "entry_point" "output_labeling_schema" "qir_profiles"="adaptive_profile" "required_num_qubits"="1" "required_num_results"="1" }
attributes #1 = { "irreversible" }
!llvm.module.flags = !{!0, !1, !2, !3, !4}
!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
!4 = !{i32 5, !"int_computations", !{!"i64"}}
