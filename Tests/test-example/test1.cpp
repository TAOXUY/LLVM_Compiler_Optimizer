int foo(volatile unsigned a) {
	volatile unsigned x = 10;
	while (a > 5) {
		a--;
		x = x + 5;
	}
	return x+a;
}
