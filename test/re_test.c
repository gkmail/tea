#include <tea.h>

int main(int argc, char **argv)
{
	T_Re *re;
	char *re_str;
	char *test_str;
	int start, end;
	T_Result r;

	/*Match digit.*/
	re_str = "[[:digit:]]+";
	re = t_re_create(re_str, -1, 0, NULL);
	test_str = "123456";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 0) || (end != 6)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	test_str = "AAAA123456BBBB";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 4) || (end != 10)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	test_str = "hello";
	r = t_re_match(re, test_str, -1, &start, &end);
	if(r != 0){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	t_re_destroy(re);

	/*Test charset.*/
	re_str = "[abcdefgABCDEFG]+";
	re = t_re_create(re_str, -1, 0, NULL);

	test_str = "123aefz";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 3) || (end != 6)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	test_str = "123AEFz";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 3) || (end != 6)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	t_re_destroy(re);

	/*Test char range.*/
	re_str = "[b-da-bb-cc-db-d]+";
	re = t_re_create(re_str, -1, 0, NULL);

	test_str = "eadcbe";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 1) || (end != 5)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	t_re_destroy(re);

	/*Test negative.*/
	re_str = "[^a-z]+";
	re = t_re_create(re_str, -1, 0, NULL);

	test_str = "eadcbe";
	r = t_re_match(re, test_str, -1, &start, &end);
	if(r != 0){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	test_str = "ead`12`2cbe";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 3) || (end != 8)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	t_re_destroy(re);

	/*Test union.*/
	re_str = "[[:digit:]]{+}[[:alpha:]]+";
	re = t_re_create(re_str, -1, 0, NULL);

	test_str = "!!!1231AADDeadcbe";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 3) || (end != 17)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	t_re_destroy(re);

	/*Test diff.*/
	re_str = "[[:alnum:]]{-}[[:alpha:]]+";
	re = t_re_create(re_str, -1, 0, NULL);

	test_str = "!!!1234abcd";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 3) || (end != 7)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	t_re_destroy(re);

	/*Test or.*/
	re_str = "[a-c]+|[a-z]+";
	re = t_re_create(re_str, -1, 0, NULL);

	test_str = "!!!abcdef";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 3) || (end != 9)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	t_re_destroy(re);

	/*Test repeat.*/
	re_str = "[[:alpha:]]{2,3}";
	re = t_re_create(re_str, -1, 0, NULL);

	test_str = "!!!abcdef";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 3) || (end != 6)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	test_str = "!!!ab!!!";
	r = t_re_match(re, test_str, -1, &start, &end);
	if((r <= 0) || (start != 3) || (end != 5)){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	test_str = "!!!a";
	r = t_re_match(re, test_str, -1, &start, &end);
	if(r != 0){
		printf("match: \"%s\" \"%s\" failed (%d %d %d)\n", re_str, test_str, r, start, end);
		abort();
	}

	t_re_destroy(re);

	return 0;
}

