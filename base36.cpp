#include <random>
#include <iostream>
#include <functional>
#include <algorithm>
#include <bitset>
#include <cassert>

uint64_t k_base36Powers[] = {
    1ULL,
    36ULL,
    36ULL*36ULL,
    36ULL*36ULL*36ULL,
    36ULL*36ULL*36ULL*36ULL,
    36ULL*36ULL*36ULL*36ULL*36ULL,
    36ULL*36ULL*36ULL*36ULL*36ULL*36ULL,
    36ULL*36ULL*36ULL*36ULL*36ULL*36ULL*36ULL,
    36ULL*36ULL*36ULL*36ULL*36ULL*36ULL*36ULL*36ULL,
    36ULL*36ULL*36ULL*36ULL*36ULL*36ULL*36ULL*36ULL*36ULL // 10digits
};


std::string EncodeBase36(const uint64_t data, uint8_t maxOutputDigits, bool trimLeftZeroes) {
    static constexpr char sEncodingTable[] = {
        '0', '1', '2', '3', '4', '5',
        '6', '7', '8', '9', 'A', 'B',
        'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N',
        'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
    };

    uint64_t remainder = data;
    if (remainder / 36 > k_base36Powers[maxOutputDigits])
    {
        return "overflow";
    }

    const uint32_t k_MaxDigits = maxOutputDigits;

    std::string output;
    output.reserve(k_MaxDigits);

    int count = k_MaxDigits;
    while(count >= 0)
    {
        const uint64_t currentPower = k_base36Powers[count];
        const uint64_t c = remainder / currentPower;
        remainder -= c * currentPower;
        //std::cout << "remainder("<<currentPower<<"): " << remainder << " C(" << c << ")-> " << std::endl;
        
        if (!trimLeftZeroes || !output.empty() || c > 0)
        {
            output.push_back(sEncodingTable[c]);
        }
        --count;
        //std::cout << "remainder("<<currentPower<<"): " << remainder << " C(" << c << ")-> " << output.back() << std::endl;
    }

    output.shrink_to_fit();    
    return output;
}

uint64_t DecodeBase36(const std::string data) {
    static constexpr char sEncodingTable[] = {
        '0', '1', '2', '3', '4', '5',
        '6', '7', '8', '9', 'A', 'B',
        'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N',
        'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
    };
    
    uint64_t output = 0;
    //std::cout << std::endl;

    int count = data.size() - 1;
    for(char c : data)
    {
        const uint64_t currentPower = k_base36Powers[count];
        //std::cout << "C: " << c << " ("<< currentPower << ")-> ";
        if (c >= '0' && c <= '9') {
            const uint64_t value = (c - '0') * currentPower;
            //std::cout << value;
            output +=  value;
        } else if (c >= 'A' && c <= 'Z') {
            const uint64_t value = (c - 'A' + 10) * currentPower;
            //std::cout << value;
            output += value;
        } else {
            std::cerr<<c<<std::endl;
            assert(false);
        }
        //std::cout << std::endl;
        --count;
    }

    return output;
}

std::string EncodeBase32(const uint32_t data) {
    static constexpr char sEncodingTable[] = {
      'A', 'B', 'C', 'D', 'E', 'F',
      'G', 'H', 'I', 'J', 'K', 'L',
      'M', 'N', 'O', 'P', 'Q', 'R',
      'S', 'T', 'U', 'V', 'W', 'X',
      'Y', 'Z', '0', '1', '2', '3',
      '4', '5', '6', '7', '8', '9', 
    };

    const uint32_t k_NumBits = 5;
    const uint32_t k_MaxValue = (1 << k_NumBits) - 1;

    std::string output;
    output.reserve(sizeof(data) * 8 / k_NumBits);

    const uint32_t mask = k_MaxValue;
    const uint8_t b1 = (data >> (30 - k_NumBits)) & mask; 
    const uint8_t b2 = (data >> (30 - k_NumBits*2)) & mask;
    const uint8_t b3 = (data >> (30 - k_NumBits*3)) & mask;
    const uint8_t b4 = (data >> (30 - k_NumBits*4)) & mask;
    const uint8_t b5 = (data >> (30 - k_NumBits*5)) & mask;
    const uint8_t b6 = (data >> (30 - k_NumBits*6)) & mask;

    output.push_back(sEncodingTable[b1 - 1]);
    output.push_back(sEncodingTable[b2 - 1]);
    output.push_back(sEncodingTable[b3 - 1]);
    output.push_back(sEncodingTable[b4 - 1]);
    output.push_back(sEncodingTable[b5 - 1]);
    output.push_back(sEncodingTable[b6 - 1]);
    assert(output.size() == sizeof(data) * 8 / k_NumBits);

    // std::cout << std::endl;
    // std::cout << "b0: " << std::bitset<k_NumBits>(b1) << " -> " << (int)b1 << " -> " << output[0] << std::endl;
    // std::cout << "b1: " << std::bitset<k_NumBits>(b2) << " -> " << (int)b2 << " -> " << output[1] << std::endl;
    // std::cout << "b2: " << std::bitset<k_NumBits>(b3) << " -> " << (int)b3 << " -> " << output[2] << std::endl;
    // std::cout << "b3: " << std::bitset<k_NumBits>(b4) << " -> " << (int)b4 << " -> " << output[3] << std::endl;
    // std::cout << "b4: " << std::bitset<k_NumBits>(b5) << " -> " << (int)b5 << " -> " << output[4] << std::endl;
    // std::cout << "b5: " << std::bitset<k_NumBits>(b6) << " -> " << (int)b6 << " -> " << output[5] << std::endl;

    return output;

}

uint32_t DecodeBase32(const std::string& data) {
    static constexpr char sEncodingTable[] = {
      'A', 'B', 'C', 'D', 'E', 'F',
      'G', 'H', 'I', 'J', 'K', 'L',
      'M', 'N', 'O', 'P', 'Q', 'R',
      'S', 'T', 'U', 'V', 'W', 'X',
      'Y', 'Z', '0', '1', '2', '3',
      '4', '5', '6', '7', '8', '9', 
    };

    const uint32_t k_MaxValue = 36;
    uint32_t output = 0;

    int count = 1;
    for(char c : data)
    {
        auto cIt = std::find(std::begin(sEncodingTable), std::end(sEncodingTable), c);
        if (cIt != std::end(sEncodingTable))
        {
            const uint32_t val = std::distance(std::begin(sEncodingTable), cIt) + 1;
            //std::cout << std::endl << c << " -> " << std::bitset<5>(c) << " -> " << val << std::endl;
            output |= val << 30 - 5 * count;
        }
        ++count;
    }

    return output;

}


#include <chrono>

int main(int, char**)
{

  	std::random_device rd;
	std::minstd_rand gen(rd());
	std::uniform_int_distribution<uint64_t> dist;

    uint64_t randomNum = dist(gen);
    randomNum = randomNum & 0x0FFFFFFFFF;

    std::cout << "Hash: " << randomNum << " -> " << std::bitset<36>(randomNum) << std::endl;

    const std::string encodedBase36 =  EncodeBase36(randomNum, 9, true);
    std::cout <<"base36Enc: " << encodedBase36 << std::endl;
    std::cout <<"base36Dec: " << DecodeBase36(encodedBase36) << std::endl;

    return 0;
}
