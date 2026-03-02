OPENQASM 2.0;
include "qelib1.inc";

qreg q[4];
h q[0];
h q[1];
h q[2];
h q[3];
t q[0];
cx q[0],q[1];
t q[1];
