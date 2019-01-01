#include <iostream>
#include <vector>
#include <math.h>
#include <iomanip>

int beta;
int sub = 512;
int sub2 = 1;

float f(float x)
{
	return 1/(1/298.15+1.0/beta*logf(x/(float)(4096-x)))-273.15;
}

int main(int argc, char * argv[])
{
	if (argc != 2) {
		std::cout << argv[0] << " BETA\n";
		return -1;
	}
	beta = atoi(argv[1]);

	std::vector<std::pair<int, int>> table;
	int start_x = 1035;
	int start_y = roundf(f(start_x) * 10 * sub);
	table.push_back(std::make_pair(start_x, start_y));

	int max_diff_y = 0, max_diff_x = 0;
	int max_diff_y_at;
	int max_prod = 0;
	std::pair<int, int> max_prod_at;
	
	std::cout << "const uint16_t temp_table[][2] = {\n";
	table.push_back(std::make_pair(start_x, start_y));
	std::cout << "\t{ 0x" << std::hex << std::setfill('0') << std::setw(4) << (table.back().first | ((table.back().second & 0x70000) >> 3)) << ", 0x" << std::setfill('0') << std::setw(4) << (table.back().second & 0xffff) << " },\n";
	std::cerr << "first last_y is " << start_y << std::endl;

	for (int i = start_x + 1; i < 3158; i++) {
	//for (int i = 1300; i < 3000; i++) {
		int y = roundf(f(i) * 10 * sub);
		//int a = ((y - start_y) * sub2 + (i - start_x) / 2) / (i - start_x); // floor works better here than correct rounding???
		int a = (y - start_y) * sub2 / (i - start_x);
		if (y - start_y < max_diff_y) {
			max_diff_y = y - start_y;
			max_diff_y_at = i;
		}
		if (i - start_x > max_diff_x)
			max_diff_x = i - start_x;

		for (int x = start_x; x <= i; x++) {
			if (roundf(f(x) * 10) != (start_y + ((x - start_x) * a + sub2/2) / sub2 + sub/2) / sub) {
			//if (roundf(f(x) * 10) != (start_y + ((x - start_x) * (y - start_y) / (i - start_x)) + sub/2) / sub) {
				table.push_back(std::make_pair(i - 1, roundf(f(i - 1) * 10 * sub)));
				std::cerr << roundf(f(x) * 10) << ", " << (start_y + ((x - start_x) * a + sub2/2) / sub2 + sub/2) / sub << " ";
				std::cerr << "x " << x << " start_x " << start_x << " ";
				std::cerr << table.back().first << ", " << table.back().second / sub << std::endl;
				std::cout << "\t{ 0x" << std::hex << std::setfill('0') << std::setw(4) << (table.back().first | ((table.back().second & 0x70000) >> 3)) << ", 0x" << std::setfill('0') << std::setw(4) << (table.back().second & 0xffff) << " },\n";
				start_x = table.back().first;
				start_y = table.back().second;
				break;
			}
			/*if (- (x - start_x) * (y - start_y) > max_prod) {
				max_prod = -(x - start_x) * (y - start_y);
				max_prod_at = std::make_pair(i, x);
				if (i == 1641 && x == 1641)
					std::cout << std::dec << "max prod at " << x << " " << start_x << " " << y << " " << start_y << std::endl;
			}*/
		}
	}
	std::cout << "};\n";
	std::cerr << "got " << std::dec << table.size() << " elements. max x: " << max_diff_x << " max y: " << -max_diff_y << " at " << std::dec << max_diff_y_at << " max prod " << max_prod << " at " << max_prod_at.first << " " << max_prod_at.second << "\n";
}
