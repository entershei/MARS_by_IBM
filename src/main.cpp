#include "tables.h"
#include <utility>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;

vector<uint32_t> keys(40, 0);

uint32_t to_num_32(const string &s) {
    uint32_t t = 0;
    for (char c : s) {
        t <<= 1UL;
        if (c == '1') {
            t += 1;
        }
    }
    return t;
}

string to_str_32(uint32_t n) {
    string res;

    while (n > 0) {
        if (n % 2) {
            res += '1';
        } else {
            res += '0';
        }
        n /= 2;
    }

    while (res.length() != 32) {
        res += '0';
    }

    reverse(res.begin(), res.end());
    return res;
}

uint32_t left_rotation(uint32_t n, uint32_t rot) {
    rot %= 32;
    if (rot == 0) {
        return n;
    }
    uint32_t tail = n >> (32 - rot);
    n <<= rot;
    n += tail;
    return n;
}

uint32_t right_rotation(uint32_t n, uint32_t rot) {
    rot %= 32;
    if (rot == 0) {
        return n;
    }
    uint32_t head = n << (32 - rot);
    n >>= rot;
    n += head;
    return n;
}

int mod_15(int t) {
    t %= 15;
    if (t < 0) {
        t += 15;
    }
    return t;
}

uint32_t get_low_bits(uint32_t x, size_t k) {
    return (x << (32 - k)) >> (32 - k);
}

uint32_t get_from_S(uint32_t x) {
    if (x < 256) {
        return S0[x];
    } else {
        return S1[x - 256];
    }
}

string to_128_string(const string &m) {
    stringstream ss, D, C, BB, A;
    for (size_t i = 0; i < 128 - m.length(); ++i) {
        ss << "0";
    }
    ss << m;

    return ss.str();
}

vector<uint32_t> string_message(string &m) {
    vector<uint32_t> block_message(4, 0);

    string s = to_128_string(m);

    stringstream D, C, BB, A;
    for (size_t i = 0; i < 32; ++i) {
        D << s[i];
    }
    block_message[3] = to_num_32(D.str());

    for (size_t i = 32; i < 64; ++i) {
        C << s[i];
    }
    block_message[2] = to_num_32(C.str());

    for (size_t i = 64; i < 96; ++i) {
        BB << s[i];
    }
    block_message[1] = to_num_32(BB.str());

    for (size_t i = 96; i < 128; ++i) {
        A << s[i];
    }
    block_message[0] = to_num_32(A.str());

    return block_message;
}

void string_key(string &k, vector<uint32_t> &temp_key) {
    if ((k.length() > 128 && k.length() % 32 != 0) || k.length() > 448) {
        cerr << "Incorrect key";
    } else {
        stringstream ss;
        for (int i = 0; i < 128 - (int)k.length(); ++i) {
            ss << "0";
        }
        ss << k;
        k = ss.str();
    }

    for (size_t i = 0; i < k.length() / 32; ++i) {
        stringstream temp;
        for (size_t j = 0; j < 32; ++j) {
            temp << k[((k.length() / 32) - 1 - i) * 32 + j];
        }
        uint32_t num = to_num_32(temp.str());
        keys[i] = num;
        temp_key[i] = num;
    }
    temp_key[k.length() / 32] = k.length() / 32;
}

uint32_t generate_mask(uint32_t k) {
    string w_str = to_str_32(k);
    vector<size_t> bit_reset;

    char last_c = w_str[0];
    size_t num_last_c = 1;

    for (size_t pos = 1; pos < w_str.length(); ++pos) {
        char c = w_str[pos];
        if (c != last_c && num_last_c >= 10) {
            bit_reset.emplace_back(pos - num_last_c);
            bit_reset.emplace_back(pos - 1);
        }
        if (c != last_c) {
            last_c = c;
            num_last_c = 1;
        } else {
            ++num_last_c;
        }
    }

    string mask = "00000000000000000000000000000000";

    for (size_t i = 0; i < bit_reset.size(); i += 2) {
        size_t l = bit_reset[i];
        size_t r = bit_reset[i + 1];
        for (size_t pos = l + 1; pos < r; ++pos) {
            mask[pos] = '1';
        }
    }

    //cout << mask;
    return to_num_32(mask);
}

void key_extension(string &key) {
    vector<uint32_t> temp_key(15, 0);
    string_key(key, temp_key);

    // 4 раунда
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 15; ++i) {
            temp_key[i] =
                    temp_key[i] ^ (left_rotation((temp_key[mod_15(i - 7)] ^ temp_key[mod_15(i - 2)]), 3) ^ (4 * i + j));
        }
        for (size_t r = 0; r < 4; ++r) {
            for (int i = 0; i < 15; ++i) {
                uint32_t t_i = temp_key[mod_15(i - 1)];
                temp_key[i] = left_rotation(temp_key[i] + get_from_S(get_low_bits(t_i, 9)), 9);
            }
            for (size_t n = 0; n < 10; ++n) {
                keys[10 * j + n] = temp_key[(4 * n) % 15];
            }
        }
    }
    // Пробегаемся по шестнадцати словам, используемыми для перемножения(K[5],K[7]…K[35]) и модифицируем их для того,
    // чтобы они соответствовали двум свойствам (нечетное, нет 10ти и более 0 или 1 подряд)
    for (int i = 5; i < 36; i += 2) {
        // Запоминаем два младших разряда
        uint32_t j = keys[i] & 3;
        uint32_t w = keys[i] | 3;
        // Генерируем маску
        uint32_t mask = generate_mask(w);
        // Используем таблица корректирующих значений B
        uint32_t b = B[j];
        uint32_t low_5_bit = get_low_bits(keys[i - 1], 5);
        uint32_t p = left_rotation(b, low_5_bit);
        // Окончательно, делается XOR шаблона p на исходное слово, при учете маски М
        keys[i] = w ^ (p & mask);
    }
}

vector<uint32_t> preliminary_imposition_of_key(vector<uint32_t> message) {
    vector<uint32_t> res(4);
    for (int i = 0; i < 4; ++i) {
        res[i] = message[i];
    }

    for (uint i = 0; i < 4; ++i) {
        res[i] += keys[i];
    }

    return res;
}

uint32_t get_byte(uint32_t n, size_t ind) {
    return (n >> (ind * 8UL)) & ((1UL << 8UL) - 1);
}

void rotation(vector<uint32_t> &coding_message) {
    vector<uint32_t> res = coding_message;
    for (int i = 0; i < 4; ++i) {
        res[i] = coding_message[(i + 1) % 4];
    }
    coding_message = res;
}

void direct_mixing(vector<uint32_t> &coding_message, size_t num_mixing) {
    coding_message[1] = coding_message[1] ^ S0[get_byte(coding_message[0], 0)];
    coding_message[1] = coding_message[1] + S1[get_byte(coding_message[0], 1)];
    coding_message[2] = coding_message[2] + S0[get_byte(coding_message[0], 2)];
    coding_message[3] = coding_message[3] ^ S1[get_byte(coding_message[0], 3)];
    coding_message[0] = right_rotation(coding_message[0], 24);

    if (num_mixing == 0 || num_mixing == 4) {
        coding_message[0] += coding_message[3];
    }

    if (num_mixing == 1 || num_mixing == 5) {
        coding_message[0] += coding_message[1];
    }

    rotation(coding_message);
}

vector<uint32_t> e_function(uint32_t in, uint32_t key1, uint32_t key2) {
    uint32_t L = 0, R = 0, M = 0;

    M = in + key1;
    R = left_rotation(in, 13) * key2;
    //assert((key2 & 1UL) == 0);

    uint32_t i = get_low_bits(M, 9);
    L = get_from_S(i);
    R = left_rotation(R, 5);
    uint32_t r = get_low_bits(R, 5);

    M = left_rotation(M, r);

    L ^= R;
    R = left_rotation(R, 5);
    L ^= R;

    r = get_low_bits(R, 5);
    L = left_rotation(L, r);

    return {L, M, R};
}

void kernel(vector<uint32_t> &coding_message, size_t i) {
    const vector<uint32_t> res_e_func = e_function(coding_message[0], keys[2 * i + 4], keys[2 * i + 5]);

    coding_message[0] = left_rotation(coding_message[0], 13);
    coding_message[2] += res_e_func[1];

    if (i < 8) {
        coding_message[1] += res_e_func[0];
        coding_message[3] ^= res_e_func[2];
    } else {
        coding_message[3] += res_e_func[0];
        coding_message[1] ^= res_e_func[2];
    }

    rotation(coding_message);
}

void back_mixing(vector<uint32_t> &coding_message, size_t ind) {
    if (ind == 2 || ind == 6) {
        coding_message[0] -= coding_message[3];
    } else if (ind == 3 || ind == 7) {
        coding_message[0] -= coding_message[1];
    }

    coding_message[1] ^= S1[get_byte(coding_message[0], 0)];
    coding_message[2] -= S0[get_byte(coding_message[0], 3)];
    coding_message[3] -= S1[get_byte(coding_message[0], 2)];
    coding_message[3] ^= S0[get_byte(coding_message[0], 1)];

    coding_message[0] = left_rotation(coding_message[0], 24);

    rotation(coding_message);
}

string encoder(string message, string key) {
    vector<uint32_t> block_message = string_message(message);

    key_extension(key);

    vector<uint32_t> coding_message = preliminary_imposition_of_key(block_message);

    for (size_t i = 0; i < 8; ++i) {
        direct_mixing(coding_message, i);
    }

    for (size_t i = 0; i < 16; ++i) {
        kernel(coding_message, i);
    }

    for (size_t i = 0; i < 8; ++i) {
        back_mixing(coding_message, i);
    }

    for (int i = 0; i < 4; ++i) {
        coding_message[i] -= keys[36 + i];
    }

    string encoded_message;
    for (int i = 3; i >= 0; --i) {
        encoded_message += to_str_32(coding_message[i]);
    }
    return encoded_message;
}

void reverse_rotation(vector<uint32_t> &message) {
    vector<uint32_t> res(4, 0);

    for (int i = 1; i < 4; ++i) {
        res[i] = message[i - 1];
    }
    res[0] = message[3];

    message = res;
}

void decode_direct_mixing(vector<uint32_t> &message, int i) {
    reverse_rotation(message);

    message[0] = right_rotation(message[0], 24);

    message[3] ^= S0[get_byte(message[0], 1)];
    message[3] += S1[get_byte(message[0], 2)];
    message[2] += S0[get_byte(message[0], 3)];
    message[1] ^= S1[get_byte(message[0], 0)];

    if (i == 2 || i == 6) {
        message[0] += message[3];
    } else if (i == 3 || i == 7) {
        message[0] += message[1];
    }
}

void decode_kernel(vector<uint32_t> &message, int ind) {
    reverse_rotation(message);

    message[0] = right_rotation(message[0], 13);

    vector<uint32_t> res_e_func = e_function(message[0], keys[2 * ind + 4], keys[2 * ind + 5]);

    message[2] -= res_e_func[1];

    if (ind < 8) {
        message[1] -= res_e_func[0];
        message[3] ^= res_e_func[2];
    } else {
        message[1] ^= res_e_func[2];
        message[3] -= res_e_func[0];
    }
}

void decode_back_mixing(vector<uint32_t> &message, int ind) {
    reverse_rotation(message);

    if (ind == 0 || ind == 4) {
        message[0] -= message[3];
    } else if (ind == 1 || ind == 5) {
        message[0] -= message[1];
    }

    message[0] = left_rotation(message[0], 24);

    message[3] ^= S1[get_byte(message[0], 3)];
    message[2] -= S0[get_byte(message[0], 2)];
    message[1] -= S1[get_byte(message[0], 1)];
    message[1] ^= S0[get_byte(message[0], 0)];
}

string decoder(string encoded_message, string key) {
    vector<uint32_t> message = string_message(encoded_message);
    key_extension(key);

    for (size_t i = 0; i < 4; ++i) {
        message[i] += keys[36 + i];
    }

    for (int i = 7; i >= 0; --i) {
        decode_direct_mixing(message, i);
    }

    for (int i = 15; i >= 0; --i) {
        decode_kernel(message, i);
    }

    for (int i = 7; i >= 0; --i) {
        decode_back_mixing(message, i);
    }

    for (int i = 0; i < 4; ++i) {
        message[i] -= keys[i];
    }

    string decoded_message;
    for (int i = 3; i >= 0; --i) {
        decoded_message += to_str_32(message[i]);
    }
    return decoded_message;
}

void test(const string &message, const string &key) {
    cout << "=================================\n";

    string encoded_message = encoder(message, key);
    string decoded_message = decoder(encoded_message, key);

    const string message_128 = to_128_string(message);
    cout << "Message:" << endl << message_128 << endl;
    cout << "Key:" << endl << ((key.size() > 128) ? key : to_128_string(key)) << endl;
    cout << "Encoded message:" << endl << encoded_message << endl;
    cout << "Decoded message:" << endl << decoded_message << endl;

    cout << "\n";
    if (decoded_message == message_128) {
        cout << "SUCCESSFULLY" << endl;
    } else {
        cout << "FAILED" << endl;
    }
    cout << "=================================\n";

    cout << "\n\n";
}

void tests() {
    freopen("tests_out", "w", stdout);

    test("01000010010000100100001001000010010000100100001001000010010000100100001001000010010000100100001001000010010000100100001001000010", "01000010010000100100001001000010010000100100001001000010010000100100001001000010010000100100001001000010010000100100001001000010");
    test("1001001100101100000001011010010", "1001001100101100000001011010010");

    test("10000111110000010100001111100000101000011111000001010000111110000010", "1");

    test("11111111111111000000011111111111111100000000000000111111111111101111111111111100000001111111111111110000000000000011111111111110",
         "11111111111111000000011111111111111100000000000000111111111111101111111111111100000001111111111111110000000000000011111111111110");

    test("011111111111111111111", "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");

    test("0", "0");
    test("1", "0");
    test("0", "1");
    test("1", "1");

    test("1111111111111111111111111111111111111111111111111111111111111111", "0");
    test("1111111111111111111111111111111111111111111111111111111111111111", "1");
    test("1111111111111111111111111111111111111111111111111111111111111111", "111111111111111111");
    test("1111111111111111111111111111111111111111111111111111111111111111", "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void create_tables() {
    for (int i = 0; i < 256; ++i) {
        S0[i] = S[i];
    }

    for (int i = 0; i < 256; ++i) {
        S1[i] = S[i + 256];
    }
}

int main() {
    create_tables();
    string message;
    string key;

    cout << "Enter message[1..128 bit] and key[less then 128 bit or mod 32 == 0]:" << endl;

    cin >> message;
    cin >> key;

    test(message, key);

    //tests();

    return 0;
}