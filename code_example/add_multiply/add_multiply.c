extern int multiply(int a, int b) __attribute__((export_name("multiply")));

int add(int a, int b) __attribute__((export_name("add")))
{
	return multiply(a, b) + a + b;
}
