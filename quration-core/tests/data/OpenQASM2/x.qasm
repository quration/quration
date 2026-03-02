OPENQASM 2.0;
qreg q[2];
creg c[2];
U(pi,0,pi) q[0];
measure q[0] -> c[0];
measure q[1] -> c[1];
