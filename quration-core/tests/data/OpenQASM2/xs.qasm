OPENQASM 2.0;
qreg q[3];
creg c[3];
U(pi,0,pi) q;
measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];
