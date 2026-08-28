int a, b;
int main()
{
	b = 5;
	println(b);
	a = b++ + b;
	println(a);
	println(b);
	return 0;
}