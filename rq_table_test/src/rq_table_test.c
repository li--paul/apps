/*
 ============================================================================
 Name        : rq_table_test.c
 Author      : tommy
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef signed int mp3d_fixed_t;
#define RQ_BITDEPTH 		8	//9   //11 is original one. table is 0-8191.
struct fixedfloat {
	unsigned int mantissa;
	unsigned int exponent;

};

struct fixedfloat rq_table[] = {
//size 8207 or small
#include "rq_table_all.dat"
		};

#define TEST_START_INDEX		(1<<RQ_BITDEPTH)

static void t_get_table(int idx, int* m, int* e) {
	*m = rq_table[idx].mantissa;
	*e = rq_table[idx].exponent;
}

static int t_get_mad(int idx, int* m, int* e) {
	int value2, value = idx;
	int i;
	struct fixedfloat const *power, *power2;
	int exp_int = 0;
	int requantized;

	value2 = value;
	i = 0;
	while (value2) {
		i++;
		value2 = value2 >> 1;
	}
	power = &rq_table[(int) (value >> (i - RQ_BITDEPTH))];
	power2 = &rq_table[1 << (i - RQ_BITDEPTH)];
	exp_int += power->exponent;
	exp_int += power2->exponent;

	requantized = (int) (((int64_t) power->mantissa * power2->mantissa) >> 28);
	//fix the requantized
	requantized += 9500 * (value & ((1 << (i - RQ_BITDEPTH)) - 1));

	*m = requantized;
	*e = exp_int;
	return (idx & ((1 << (i - RQ_BITDEPTH)) - 1));
}

static int t_get_mad_mod1(int idx, int* m, int* e) {
	int value2, value = idx;
	int i;
	struct fixedfloat const *power, *power2;
	int exp_int = 0;
	int requantized;
	int magic = 9500;
	struct fixedfloat const *power3;

	value2 = value;
	i = 0;
	while (value2) {
		i++;
		value2 = value2 >> 1;
	}
	power = &rq_table[(int) (value >> (i - RQ_BITDEPTH))];
	power2 = &rq_table[1 << (i - RQ_BITDEPTH)];
	exp_int += power->exponent;
	exp_int += power2->exponent;

	power3 = &rq_table[(int) (value >> (i - RQ_BITDEPTH)) + 1];

	if (power->exponent == power3->exponent) {

	}

	/*
	 *  2049 - 17340
	 2896 - 19463
	 2898 - 9734
	 5795 - 12261
	 5797 - 6132
	 */

	if (value > 5797)
		magic = 6160;
	else if (value > 2898)
		magic = 10261;
	else if (value > 2048)
		magic = 18000;

	requantized = (int) (((int64_t) power->mantissa * power2->mantissa) >> 28);
	//fix the requantized
	requantized += magic * (value & ((1 << (i - RQ_BITDEPTH)) - 1));

	*m = requantized;
	*e = exp_int;
	return (idx & ((1 << (i - RQ_BITDEPTH)) - 1));

	/*
	 *
	 mad original is
	 Average percent diff is 0.009349%
	 Max percent diff is 0.037882%
	 2 magic
	 Average percent diff is 0.004710%
	 Max percent diff is 0.029420%
	 3 magic
	 Average percent diff is 0.003782%
	 Max percent diff is 0.029420%
	 all 4 magic
	 Average percent diff is 0.002085%
	 Max percent diff is 0.015487%

	 */
}

// Only half table when 1k-2k.
static inline int t_get_1k_2k(int idx, int* m, int* e) {

	unsigned int m0, e0;
	unsigned int m1, e1;

	if (idx > 2048) {
		exit(1);
	}

	m0 = rq_table[idx & 0xfffe].mantissa;
	e0 = rq_table[idx & 0xfffe].exponent;
	m1 = rq_table[(idx & 0xfffe) + 2].mantissa;
	e1 = rq_table[(idx & 0xfffe) + 2].exponent;

	if (idx & 1) {
		if (e0 == e1) {
			*m = (m0 + m1) / 2;
			*e = e0;
		} else {
			printf(" e0 diff e1 \n");
			printf(" %x %x %x %x ", m0, e0, m1, e1);
			*m = ((m0 >> (e1 - e0)) + m1) / 2;
			*e = e1;
		}
	} else {
		*m = m0;
		*e = e0;
	}
	return 0;
}

#define RQ_BITDEPTH_TEST	11
static int t_get_mad_mod2(int idx, int* m, int* e) {
	unsigned int value2, value = idx;
	int i;
	struct fixedfloat power, power1, power2;
	unsigned int exp_int = 0;
	unsigned int requantized;

	if (value > 1024 && value < 2048) {
		return t_get_1k_2k(idx, m, e);
	}

	value2 = value;
	i = 0;
	while (value2) {
		i++;
		value2 = value2 >> 1;
	}

#if 0
	t_get_1k_2k((int) (value >> (i - 11)), &power.mantissa, &power.exponent);
	power2 = rq_table[1 << (i - 11)];
	exp_int += power.exponent;
	exp_int += power2.exponent;

	requantized = (int) (((int64_t) power.mantissa * power2.mantissa) >> 28);
	//fix the requantized
	requantized += 9500 * (value & ((1 << (i - 11)) - 1));
#else
	t_get_1k_2k((int) (value >> (i - 11)), &power.mantissa, &power.exponent);
	t_get_1k_2k((int) (value >> (i - 11)) + 1, &power1.mantissa,
			&power1.exponent);

	if (power.exponent == power1.exponent) {
//		printf("before %x, %d, %d.\n", power.mantissa, (value & ((1 << (i - 11)) - 1)),  (i - 11));
//		printf("before %x, %d, %d.\n", power1.mantissa, (value & ((1 << (i - 11)) - 1)),  (i - 11));
//		printf("%x\n", (power1.mantissa - power.mantissa)>> (i - 11));
//		power.mantissa = power.mantissa  + ((power1.mantissa - power.mantissa) /** (value & ((1 << (i - 11)) - 1))*/) >> (i - 11);
		power.mantissa += (((power1.mantissa - power.mantissa)
				* (value & ((1 << (i - 11)) - 1))) >> (i - 11));
//		printf("after %x\n", power.mantissa);

		power2 = rq_table[1 << (i - 11)];
		exp_int += power.exponent;
		exp_int += power2.exponent;

		requantized =
				(int) (((int64_t) power.mantissa * power2.mantissa) >> 28);
	} else {
		printf("\nfire \n");
		power2 = rq_table[1 << (i - 11)];
		exp_int += power.exponent;
		exp_int += power2.exponent;

		requantized =
				(int) (((int64_t) power.mantissa * power2.mantissa) >> 28);
		//fix the requantized
		requantized += 9500 * (value & ((1 << (i - 11)) - 1));

	}

#endif

	*m = requantized;
	*e = exp_int;
	return (idx & ((1 << (i - RQ_BITDEPTH)) - 1));

	/*
	 *
	 Average percent diff is 0.008014%
	 Max percent diff is 0.037893%

	 *
	 */
}

static int t_get_mad_mod3(int idx, int* m, int* e) {
	unsigned int value2, value = idx;
	int i;
	struct fixedfloat power, power1, power2;
	unsigned int exp_int = 0;
	unsigned int requantized;

	if (value > 1024 && value < 2048) {
		return t_get_1k_2k(idx, m, e);
	}

	value2 = value;
	i = 0;
	while (value2) {
		i++;
		value2 = value2 >> 1;
	}

	t_get_1k_2k((int) (value >> (i - 11)), &power.mantissa, &power.exponent);
	t_get_1k_2k((int) (value >> (i - 11)) + 1, &power1.mantissa,
			&power1.exponent);

	if (power.exponent == power1.exponent) {
		power.mantissa += (((power1.mantissa - power.mantissa)
				* (value & ((1 << (i - 11)) - 1))) >> (i - 11));

		power2 = rq_table[1 << (i - 11)];
		exp_int += power.exponent;
		exp_int += power2.exponent;

		requantized =
				(int) (((int64_t) power.mantissa * power2.mantissa) >> 28);
	} else {
		printf("\nfire \n");
		power1.mantissa <<= (power1.exponent - power.exponent);
		power.mantissa += (((power1.mantissa - power.mantissa)
				* (value & ((1 << (i - 11)) - 1))) >> (i - 11));

		power2 = rq_table[1 << (i - 11)];
		exp_int += power.exponent;
		exp_int += power2.exponent;

		requantized =
				(int) (((int64_t) power.mantissa * power2.mantissa) >> 28);
	}

	*m = requantized;
	*e = exp_int;
	return (idx & ((1 << (i - RQ_BITDEPTH)) - 1));

	/*
	 *
	 Average percent diff is 0.008014%
	 Max percent diff is 0.037893%

	 Average percent diff is 0.000005%
	 Max percent diff is 0.000021%
	 *
	 */
}

#define TEST_BITDEPTH		(RQ_BITDEPTH)		//10 testOK.
static inline int t_get_1k(int idx, int* m, int* e) {
	unsigned int m0, e0;

	if (idx > (1 << TEST_BITDEPTH)) {
		exit(1);
	}

//	printf("@%d, ", idx);
	m0 = rq_table[idx].mantissa;
	e0 = rq_table[idx].exponent;

//	printf("%x. %d\n", m0, e0);

	*m = m0;
	*e = e0;
	return 0;
}

static int t_get_mad_mod4(int idx, int* m, int* e) {
	unsigned int value2, value = idx;
	int i;
	struct fixedfloat power, power1, power2;
	unsigned int exp_int = 0;
	unsigned int requantized;

	value2 = value;
	i = 0;
	while (value2) {
		i++;
		value2 = value2 >> 1;
	}

	t_get_1k((int) (value >> (i - TEST_BITDEPTH)), &power.mantissa,
			&power.exponent);
	t_get_1k((int) (value >> (i - TEST_BITDEPTH)) + 1, &power1.mantissa,
			&power1.exponent);

//	printf("before %x, %d. %d, %d.\n", power.mantissa, power.mantissa,
//			(value & ((1 << (i - TEST_BITDEPTH)) - 1)), (i - TEST_BITDEPTH));
//	printf("before %x, %d. %d, %d.\n", power1.mantissa, power1.mantissa,
//			(value & ((1 << (i - TEST_BITDEPTH)) - 1)), (i - TEST_BITDEPTH));

	if (power.exponent != power1.exponent) {
		power1.mantissa <<= (power1.exponent - power.exponent);
	}
//	printf("before %x, %d. %d, %d.\n", power1.mantissa, power1.mantissa,
//			(value & ((1 << (i - TEST_BITDEPTH)) - 1)), (i - TEST_BITDEPTH));

	power.mantissa +=
			(((power1.mantissa - power.mantissa)
					* (value & ((1 << (i - TEST_BITDEPTH)) - 1)))
					>> (i - TEST_BITDEPTH));

	power2 = rq_table[1 << (i - TEST_BITDEPTH)];
	exp_int += power.exponent;
	exp_int += power2.exponent;

	requantized = (int) (((int64_t) power.mantissa * power2.mantissa) >> 28);
//	printf(" result is %x. %d\n", requantized, exp_int);

#if 1	//Add below to make the 'requantized' Q27, same as the ref table.
	value2 = requantized >> 16;
	i = 0;
	while (value2) {
		i++;
		value2 = value2 >> 1;
	}

//    printf("i is %d.", i);
	requantized <<= ((27 - 16) - i);
	exp_int -= ((27 - 16) - i);
//    printf("exp_int is %d.", exp_int);
#endif
	*m = requantized;
	*e = exp_int;
	return (idx & ((1 << (i - RQ_BITDEPTH)) - 1));

	/*
	 *
	 Average percent diff is 0.008014%
	 Max percent diff is 0.037893%
	 RQ_BITDEPTH == 10
	 Average percent diff is 0.000005%
	 Max percent diff is 0.000021%
	 RQ_BITDEPTH == 9
	 Average percent diff is 0.000026%
	 Max percent diff is 0.000084%
	 RQ_BITDEPTH == 8
	 Average percent diff is 0.000109%
	 Max percent diff is 0.000335%
	 *
	 */
}

void test1() {
	int i;
	int diff_m, diff_exp, diff;
	int index;

	printf(" TEST_START_INDEX is %d\n", TEST_START_INDEX);
	for (i = (TEST_START_INDEX) + 1; i < 8192; i++) {
		diff_exp = rq_table[i].exponent - rq_table[TEST_START_INDEX].exponent;

		diff = (int) (rq_table[i].mantissa << diff_exp)
				- rq_table[TEST_START_INDEX].mantissa;

		index = i - TEST_START_INDEX;
		printf("No.%04d, %x, %x, diff 0x%08x.", index + TEST_START_INDEX,
				diff_m, diff_exp, diff);

		// result is 0x1ae20 - 0x2b322.
		printf("\tstep diff is 0x%x.", diff / index);
		printf("\n");

	}
}

void test_mad_way() {
	int i;
	int index;
	int m1, e1;
	int m2, e2;
	int diff, diff_abs;
	float diff_per, diff_per_total = 0, diff_max = 0;

	printf(" TEST_START_INDEX is %d\n", TEST_START_INDEX);
	for (i = (TEST_START_INDEX) + 1; i < 8192; i++) {

		index = i - TEST_START_INDEX;
		printf("No.%04d,", index + TEST_START_INDEX);
		t_get_table(i, &m1, &e1);
		printf("\tTable is 0x%08x, %d.", m1, e1);
//		t_get_mad(i, &m2, &e2);
//		t_get_mad_mod1(i, &m2, &e2);
//		t_get_mad_mod2(i, &m2, &e2);
//		t_get_mad_mod3(i, &m2, &e2);
		t_get_mad_mod4(i, &m2, &e2);

		if (e2 < e1) {
			// Make e2 == e1, for easy compare.
//			printf("m2:%x, e2:%d\n", m2, e2);
//			printf("m1:%x, e1:%d\n", m2, e2);
			m2 >>= (e1 - e2);
			e2 = e1;
//			printf("m2:%x, e2:%d\n\n", m2, e2);
		}

		if (e2 >= e1) {
			diff = (m2 << (e2 - e1)) - m1;
			diff_abs = abs(diff);
			diff_per = (float) diff_abs * 100. / m1;
			if (diff_per > diff_max)
				diff_max = diff_per;
			diff_per_total += diff_per;
		} else {
			printf("Error: need %d >= %d failed.\n", e2, e1);
			exit(1);
		}
		printf("\tMad is 0x%08x, %d. diff 0x%08x 0x%08x, %f%%   ", m2, e2, diff,
				diff_abs, diff_per);
		printf("\n");

	}

	printf("Summary:\n");
	printf("Average percent diff is %f%%\n",
			diff_per_total / (8192 - TEST_START_INDEX));
	printf("Max percent diff is %f%%\n", diff_max);
	/*
	 *
	 mad original is
	 Average percent diff is 0.009349%
	 Max percent diff is 0.037882%
	 *
	 */
}

void test_mad_get9500() {
	int i;
	int index, temp;
	int m1, e1;
	int m2, e2;
	int diff, diff_abs;
	float diff_per, diff_per_total = 0, diff_max = 0;
	int count = 0;

	printf(" TEST_START_INDEX is %d\n", TEST_START_INDEX);
	for (i = (TEST_START_INDEX) + 1; i < 8192; i++) {

		index = i - TEST_START_INDEX;
		printf("No.%04d,", index + TEST_START_INDEX);
		t_get_table(i, &m1, &e1);
		printf("\tTable is 0x%08x, %d.", m1, e1);
		temp = t_get_mad(i, &m2, &e2);
		if (e2 >= e1) {
			diff = (m1 >> (e2 - e1)) - m2;
			diff_abs = abs(diff);
			if (temp) {
				if (temp)
					diff_per = (float) diff_abs / temp;
				else
					diff_per = (float) diff_abs;
				if (diff_per > diff_max)
					diff_max = diff_per;
				diff_per_total += diff_per;
				count++;
			} else {
				diff_per = 0;
			}
		} else
			exit(1);
		printf("\tMad is 0x%08x, %d. %d. diff 0x%08x 0x%08x, %f%%   ", m2, e2,
				temp, diff, diff_abs, diff_per);
		printf("\n");
	}
	printf("Summary:\n");
	printf("Average percent diff is %f%%\n", diff_per_total / count);
	printf("Max percent diff is %f%%\n", diff_max);

	/*
	 * Average percent diff is 9904.201172%
	 Max percent diff is 19463.000000%

	 RQ_BITDEPTH:10
	 1024 - 18028
	 1725 - 6873
	 3447 - 11121
	 3449 - 814
	 4095 - 1420
	 4097 - 4039
	 6895 - 3004
	 6897 - 6251


	 RQ_BITDEPTH:11
	 2049 - 17340
	 2896 - 19463
	 2898 - 9734
	 5795 - 12261
	 5797 - 6132


	 if RQ_BITDEPTH==10
	 Average percent diff is 10127.095881%
	 Max percent diff is 32734.000000%

	 */

}

/*
 *	main
 */
int main(void) {
	int i;
	puts("!!!Hello the World!!!"); /* prints !!!Hello World!!! */

//	for (i = (1 << 10); i < 8192; i++) {
//		printf("index %04d, mantissa:0x%x, exponent:%d\n", i, rq_table[i].mantissa, rq_table[i].exponent);
//	}

	test_mad_way();
//	test_mad_get9500();

	return EXIT_SUCCESS;
}
