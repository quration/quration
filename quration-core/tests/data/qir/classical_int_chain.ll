; ModuleID = 'classical_int_chain'
source_filename = "classical_int_chain"

@0 = internal constant [2 x i8] c"t\00"
@1 = internal constant [2 x i8] c"a\00"
@2 = internal constant [2 x i8] c"b\00"

define void @main() #0 {
entry:
  call void @__quantum__qis__x__body(ptr null)
  call void @__quantum__qis__mz__body(ptr null, ptr null)
  %0 = call i1 @__quantum__rt__read_result(ptr null)
  %1 = zext i1 %0 to i64
  %2 = add i64 %1, 1
  %3 = shl i64 %1, 2
  %4 = mul i64 %2, 3
  %5 = or i64 %3, 1
  %6 = sub i64 %4, 2
  %7 = xor i64 %5, 4
  call void @__quantum__rt__tuple_record_output(i64 2, ptr @0)
  call void @__quantum__rt__int_record_output(i64 %6, ptr @1)
  call void @__quantum__rt__int_record_output(i64 %7, ptr @2)
  ret void
}

declare void @__quantum__qis__x__body(ptr)

declare void @__quantum__qis__mz__body(ptr, ptr)

declare i1 @__quantum__rt__read_result(ptr)

declare void @__quantum__rt__tuple_record_output(i64, ptr)

declare void @__quantum__rt__int_record_output(i64, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="1" "required_num_results"="1" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
