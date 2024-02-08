int main() {
	int x = 10;
#pragma omp approx for schedule(dynamic)
	{
		for (int i = 0; i < 10; i++)
			x++;
	}
	return x;
}
