; ModuleID = 'record_multi_qubit_gates'
source_filename = "record_multi_qubit_gates"

@0 = internal constant [3 x i8] c"t5\00"
@1 = internal constant [3 x i8] c"r0\00"
@2 = internal constant [3 x i8] c"r1\00"
@3 = internal constant [3 x i8] c"r2\00"
@4 = internal constant [3 x i8] c"r3\00"
@5 = internal constant [3 x i8] c"r4\00"

define void @main() #0 {
entry:
  call void @__quantum__qis__x__body(ptr null)
  call void @__quantum__qis__y__body(ptr inttoptr (i64 1 to ptr))
  call void @__quantum__qis__cnot__body(ptr null, ptr inttoptr (i64 2 to ptr))
  call void @__quantum__qis__cy__body(ptr null, ptr inttoptr (i64 3 to ptr))
  call void @__quantum__qis__ccx__body(ptr null, ptr inttoptr (i64 1 to ptr), ptr inttoptr (i64 4 to ptr))
  call void @__quantum__qis__mz__body(ptr null, ptr null)
  call void @__quantum__qis__mz__body(ptr inttoptr (i64 1 to ptr), ptr inttoptr (i64 1 to ptr))
  call void @__quantum__qis__mz__body(ptr inttoptr (i64 2 to ptr), ptr inttoptr (i64 2 to ptr))
  call void @__quantum__qis__mz__body(ptr inttoptr (i64 3 to ptr), ptr inttoptr (i64 3 to ptr))
  call void @__quantum__qis__mz__body(ptr inttoptr (i64 4 to ptr), ptr inttoptr (i64 4 to ptr))
  call void @__quantum__rt__tuple_record_output(i64 5, ptr @0)
  call void @__quantum__rt__result_record_output(ptr null, ptr @1)
  call void @__quantum__rt__result_record_output(ptr inttoptr (i64 1 to ptr), ptr @2)
  call void @__quantum__rt__result_record_output(ptr inttoptr (i64 2 to ptr), ptr @3)
  call void @__quantum__rt__result_record_output(ptr inttoptr (i64 3 to ptr), ptr @4)
  call void @__quantum__rt__result_record_output(ptr inttoptr (i64 4 to ptr), ptr @5)
  ret void
}

declare void @__quantum__qis__x__body(ptr)

declare void @__quantum__qis__y__body(ptr)

declare void @__quantum__qis__cnot__body(ptr, ptr)

declare void @__quantum__qis__cy__body(ptr, ptr)

declare void @__quantum__qis__ccx__body(ptr, ptr, ptr)

declare void @__quantum__qis__mz__body(ptr, ptr)

declare void @__quantum__rt__tuple_record_output(i64, ptr)

declare void @__quantum__rt__result_record_output(ptr, ptr)

attributes #0 = { "entry_point" "output_labeling_schema"="schema_id" "qir_profiles"="adaptive_profile" "required_num_qubits"="5" "required_num_results"="5" }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"qir_major_version", i32 1}
!1 = !{i32 7, !"qir_minor_version", i32 0}
!2 = !{i32 1, !"dynamic_qubit_management", i1 false}
!3 = !{i32 1, !"dynamic_result_management", i1 false}
