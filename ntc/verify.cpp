#include <math.h>
#include <iostream>

#include "../temp_lookup.c"

int beta = 3950;
float f(float x)
{
	return 1/(1/298.15+1.0/beta*logf(x/(float)(4096-x)))-273.15;
}

uint16_t temp(uint16_t sum)
{
	// binary search for largest table entry that is smaller than or equal sum (floor)
	// TODO test this for all possible sum values
	uint8_t start = 0;
	uint8_t end = sizeof(temp_table) / sizeof(uint16_t) / 2 - 1;
	while (true) {
		uint8_t mid = start + end;
		uint16_t t;
		if (mid & 1)
			mid += 0x2;
		mid >>= 1;
		t = temp_table[mid][0] & 0x1fff;
		//printf("range is %d, %d, %d. comparing %d %d\n", start, mid, end, t, sum);
		if (t == sum) {
			//std::cout << "exact " << sum << " " << (((temp_table[mid][1] | ((uint32_t)(temp_table[mid][0] & 0xe000) << 3)) + 512/2) >> 9) << " " << (((temp_table[mid][1] | ((uint32_t)(temp_table[mid][0] & 0xe000) << 3)))) << "\n";
			return ((temp_table[mid][1] | ((uint32_t)(temp_table[mid][0] & 0xe000) << 3)) + 512/2) >> 9;
			break;
		}
		else if (sum < t)
			end = mid - 1;
		else
			start = mid;
		if (end <= start) {
			if (end == sizeof(temp_table) / sizeof(uint16_t) / 2 - 1) // end may not be last element! we need end+1
				end--;
			{
				uint16_t last_x = temp_table[end][0] & 0x1fff;
				uint32_t last_y = temp_table[end][1] | ((uint32_t)(temp_table[end][0] & 0xe000) << 3);
				uint16_t next_x = temp_table[end+1][0] & 0x1fff;
				uint32_t next_y = temp_table[end+1][1] | ((uint32_t)(temp_table[end+1][0] & 0xe000) << 3);
				if (last_x > sum || sum > next_x)
					std::cout << "error in binary search " << last_x << " " << sum << " " << next_x << ". end is " << (int)end << "\n";
				// a has implicit negative sign
				uint16_t a = (uint16_t)(last_y - next_y) / (next_x - last_x);
				//std::cout << "a" << a << "sum - last_x" << (sum - last_x) << "last_y" << last_y << "next_y" << next_y << "next_x" << next_x << "last_x" << last_x << std::endl;
				return (last_y - (sum - last_x) * a + 512/2) >> 9;
			}
			break;
		}
	}
}

int main()
{

	for (uint16_t sum = 1035; sum <= 3158; sum++) {
		if (temp(sum) != roundf(f(sum) * 10)) {
			// this may happen for values very near to .x5 that are not representable as floats. therefore it is indistinguishable if we have to round up- or downards
			std::cout << "error " << sum << ": " << temp(sum) << " != " << roundf(f(sum) * 10) << "\n";
			std::cout << "real value is " << f(sum) << " " << f(sum) * 10 << " " << f(sum) * 10 * 512 << std::endl;
			//break;
		}
	}
}

