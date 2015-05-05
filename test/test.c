#include <tea.h>

int main(int argc, char **argv)
{
	T_Re *re;
	int start, end;
	T_Result r;

	re = t_re_create("[[:alpha:][:digit:]]+", -1, NULL);

	r = t_re_match(re, " hello12123!!! ", -1, &start, &end);
	printf("match: %d, %d %d\n", r, start, end);

	t_re_destroy(re);
	return 0;
}

